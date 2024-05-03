#include "Scene.h"

#include <iostream>

#define EPSILON 0.0001f

using namespace AB;
using namespace std;

using IndexObjList = vector<pair<GameObject*, unsigned int>>;

// Create instance
Scene* Scene::instance;

// forward-declare node helper function for later
KDNode* CreateNode(vector<Vertex*> allVerts, IndexObjList indices, int depth, int maxDepth);

struct AB::KDNode
{
	// AABB
	glm::vec3 min, max, halfextents, center;
	IndexObjList indices;

	// children
	KDNode* left;
	KDNode* right;

	~KDNode()
	{
		// setting left and right to nullptr after deleting shouldn't matter because 
		// the whole node is being deleted anyway, but it's good practice regardless.
		if (left)
		{
			delete left; 
			left = nullptr;
		}
		if (right)
		{
			delete right; 
			right = nullptr;
		}
	};
};

Scene::~Scene()
{
	delete instance;
	instance = nullptr;
	delete root;
	root = nullptr;
}

GameObject* Scene::Add(GameObject&& obj)
{
	gameobjects.push_back(std::move(obj));
	return &gameobjects.back();
}

vector<GameObject>& Scene::GetAllObjects()
{
	return gameobjects;
}

GameObject* Scene::Find(std::string objName)
{
	for (auto& obj : gameobjects)
	{
		if (obj.GetName() == objName)
			return &obj;
	}
	return nullptr;
}

glm::vec3 GetBaryCoords(glm::vec3 origin, glm::vec3 dir, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, bool& front)
{
	glm::vec3 uvw = glm::vec3(-1, -1, -1);

	// get tri edges sharing p0
	glm::vec3 e1 = p1 - p0;
	glm::vec3 e2 = p2 - p0;

	glm::vec3 p = glm::cross(dir, e2);
	float det = glm::dot(e1, p);

	// if determinant is near 0, ray lies in tri plane.
	front = det > 0;
	if (glm::abs(det) < EPSILON)
		return uvw;

	float f = (1.f / det);
	glm::vec3 toOrigin = origin - p0;
	// calculate U and test if it's within tri bounds
	uvw.x = f * glm::dot(toOrigin, p);
	if (uvw.x < 0.f || uvw.x > 1.f)
		return uvw;

	// calculate V and test if coord is within tri bounds
	glm::vec3 q = glm::cross(toOrigin, e1);
	uvw.y = f * glm::dot(dir, q);
	if (uvw.y < 0.f || uvw.x + uvw.y > 1.f)
		return uvw;

	// calculate distance
	uvw.z = f * glm::dot(e2, q);
	return uvw;
}

// Casts a ray and returns the barycentric coords of the hit on the tri being tested against.
// Returns false if no hit. If no RaycastHit ptr was passed in, this simply returns if there was a hit.
bool Scene::Raycast(glm::vec3 origin, glm::vec3 dir, RaycastHit* hit, float maxDistance)
{
	dir = glm::normalize(dir);

	glm::vec3 resultUVW{ -1, -1, maxDistance };
	Vertex hitTri[3];
	bool front = true;

	MeshType lastType = MESH_TRI;
	float hitRadius = 0.f;

	bool successful = root ?
		RaycastTreeInternal(root, origin, dir, hit, resultUVW, hitTri, front) :
		RaycastInternal(origin, dir, hit, resultUVW, hitTri, front, lastType, hitRadius);

	// interpolate bary coords to find pos, normal, and texcoords
	if (hit && successful)
	{
		switch (lastType)
		{
		case MESH_TRI:
			hit->position = (1 - resultUVW.x - resultUVW.y) * hitTri[0].Position + resultUVW.x * hitTri[1].Position + resultUVW.y * hitTri[2].Position;
			hit->normal = glm::normalize((1 - resultUVW.x - resultUVW.y) * hitTri[0].Normal + resultUVW.x * hitTri[1].Normal + resultUVW.y * hitTri[2].Normal) * (front ? 1.f : -1.f);
			hit->texcoord = (1 - resultUVW.x - resultUVW.y) * hitTri[0].TexCoord + resultUVW.x * hitTri[1].TexCoord + resultUVW.y * hitTri[2].TexCoord;
			break;

		case MESH_SPHERE:
			hit->position = origin + (dir * resultUVW.z);
			hit->normal = (hit->position - hit->gameObject->GetWorldTM().GetTranslation()) / hitRadius;
			hit->normal *= glm::dot(dir, hit->normal) > 0.f ? -1.f : 1.f;
			break;
		}
	}

	return successful;
}

bool Scene::RaycastInternal(glm::vec3 origin, glm::vec3 dir, RaycastHit* hit, glm::vec3& resultUVW, Vertex (&hitTri)[3], bool& front, MeshType& lastType, float& hitRadius)
{
	bool successful = false;

	// loop through all tri's in scene (every 3 indices)
	for (GameObject& obj : gameobjects)
	{
		for (Mesh& m : obj.GetMeshes())
		{
			switch (m.type)
			{
			case MESH_TRI:
				for (int i = 0; i < m.indices.size(); i += 3)
				{
					// Get the first vert in this tri to get the world matrix
					Vertex p0 = m.vertices[m.indices[i]];
					Vertex p1 = m.vertices[m.indices[i + 1]];
					Vertex p2 = m.vertices[m.indices[i + 2]];

					bool thisfront = true;
					glm::vec3 uvw = GetBaryCoords(origin, dir, p0.Position, p1.Position, p2.Position, thisfront);
					if (uvw.z > EPSILON && uvw.z < resultUVW.z)
					{
						if (!hit) return true;

						lastType = MESH_TRI;
						resultUVW = uvw;
						hitTri[0] = p0;
						hitTri[1] = p1;
						hitTri[2] = p2;
						front = thisfront;

						hit->gameObject = &obj;
						hit->distance = uvw.z;
						successful = true;
					}
				}
				break;
			case MESH_SPHERE:
				glm::vec3 cToO = origin - obj.GetWorldTM().GetTranslation();

				float B = 2.f * glm::dot(dir, cToO);
				float C = glm::dot(cToO, cToO) - (m.radius * m.radius);
				float d = B * B - 4.f * C;

				if (d < 0.f) continue; // no hit

				float w1 = (-B + glm::sqrt(d)) / 2.f;
				float w2 = (-B - glm::sqrt(d)) / 2.f;
				float w0 = glm::min(w1, w2);
				if (w0 <= EPSILON) w0 = glm::max(w1, w2);
				if (w0 > EPSILON && w0 < resultUVW.z)
				{
					if (!hit) return true;

					resultUVW.z = w0;
					lastType = MESH_SPHERE;
					hitRadius = m.radius;

					hit->gameObject = &obj;
					hit->distance = w0;
					successful = true;
				}
				break;
			}
		}
	}

	return successful;
}

// Note: Due to time constraints KD trees only track primitive tri's, and not spheres.
bool Scene::RaycastTreeInternal(KDNode* node, glm::vec3 origin, glm::vec3 dir, RaycastHit* hit, glm::vec3& resultUVW, Vertex(&hitTri)[3], bool& front)
{
	bool successful = false;
	glm::vec3 dirInv = 1.f / dir;

	// test ray vs AABB (using branchless algorithm from https://tavianator.com/2015/ray_box_nan.html)
	float t1 = (node->min.x - origin.x) * dirInv.x;
	float t2 = (node->max.x - origin.x) * dirInv.x;

	float tmin = glm::min(t1, t2);
	float tmax = glm::max(t1, t2);

	for (int i = 1; i < 3; ++i) {
		t1 = (node->min[i] - origin[i]) * dirInv[i];
		t2 = (node->max[i] - origin[i]) * dirInv[i];

		tmin = glm::max(tmin, glm::min(t1, t2));
		tmax = glm::min(tmax, glm::max(t1, t2));
	}

	// if ray hits AABB, go down the stack until lead node is found
	if (tmax > glm::max(tmin, 0.f))
	{
		if (node->left)
			successful = RaycastTreeInternal(node->left, origin, dir, hit, resultUVW, hitTri, front) ? true : successful;
		if (node->right)
			successful = RaycastTreeInternal(node->right, origin, dir, hit, resultUVW, hitTri, front) ? true : successful;

		// base case; found a leaf node. 
		if (!(node->left || node->right))
		{
			// test against all tri's in leaf node
			for (int i = 0; i < node->indices.size(); i += 3)
			{
				// Get the first vert in this tri to get the world matrix
				Vertex* p0 = allVerts[node->indices[i + 0].second];
				Vertex* p1 = allVerts[node->indices[i + 1].second];
				Vertex* p2 = allVerts[node->indices[i + 2].second];

				bool thisfront = true;
				glm::vec3 uvw = GetBaryCoords(origin, dir, p0->Position, p1->Position, p2->Position, thisfront);
				if (uvw.z > EPSILON && uvw.z < resultUVW.z)
				{
					if (!hit) return true;

					resultUVW = uvw;
					hitTri[0] = *p0;
					hitTri[1] = *p1;
					hitTri[2] = *p2;
					front = thisfront;

					hit->gameObject = node->indices[i].first;
					hit->distance = uvw.z;
					successful = true;
				}
			}
		}
	}

	return successful;
}

void Scene::Render(Shader& shader)
{
	for (auto& obj : gameobjects)
	{
		obj.Draw(shader);
	}
}

void Scene::CreateTree(int maxDepth)
{
	// gather all vertices and indices in the scene
	IndexObjList allIndices;
	for (auto& obj : gameobjects)
	{
		for (auto& mesh : obj.GetMeshes())
		{
			for (unsigned int& i : mesh.indices)
				allIndices.push_back({ &obj, i + allVerts.size() });
			for (Vertex& v : mesh.vertices)
				allVerts.push_back(&v);
		}
	}
	
	// create the tree
	root = CreateNode(allVerts, allIndices, 0, maxDepth);
}

bool AABBOverlapsAABB(glm::vec4 minA, glm::vec4 maxA, glm::vec4 minB, glm::vec4 maxB)
{
	for (int i = 0; i < 3; i++)
	{
		if (minA[i] > maxB[i] || minB[i] > maxA[i])
			return false;
	}
	return true;
}

bool SphereOverlapsAABB(glm::vec3 c, float r, glm::vec3 minA, glm::vec3 maxA)
{
	float d = 0;
	for (int i = 0; i < 3; i++)
	{
		float e;
		if ((e = c[i] - minA[i]) < 0)
		{
			if (e < -r) return false;
			d += e * e;
		}	
		else if ((e = c[i] - maxA[i]) > 0)
		{
			if (e > r) return false;
			d += e * e;
		}
	}
	return d <= r * r;
}

// Code adapted from https://omnigoat.github.io/2015/03/09/box-triangle-intersection/
int TriOverlapsAABB(glm::vec4 boxMin, glm::vec4 boxMax, Vertex* triVerts[3])
{
	glm::vec4 v0 = glm::vec4(triVerts[0]->Position, 1);
	glm::vec4 v1 = glm::vec4(triVerts[1]->Position, 1);
	glm::vec4 v2 = glm::vec4(triVerts[2]->Position, 1);
	glm::vec4 e0 = v1 - v0;
	glm::vec4 e1 = v2 - v1;
	glm::vec4 e2 = v0 - v2;

	glm::vec4 triMin = glm::min(glm::min(v0, v1), v2);
	glm::vec4 triMax = glm::max(glm::max(v0, v1), v2);

	// First test AABBS before getting more complicated
	if (!AABBOverlapsAABB(boxMin, boxMax, triMin, triMax))
		return false;

	// triangle-normal
	auto n = glm::vec4(glm::normalize(triVerts[0]->Normal + triVerts[1]->Normal + triVerts[2]->Normal), 0);

	// p & delta-p
	auto p = boxMin;
	auto dp = boxMax - p;

	// test for triangle-plane/box overlap
	auto c = glm::vec4(
		n.x > 0.f ? dp.x : 0.f,
		n.y > 0.f ? dp.y : 0.f,
		n.z > 0.f ? dp.z : 0.f, 1.f);

	auto d1 = glm::dot(n, c - v0);
	auto d2 = glm::dot(n, dp - c - v0);

	if ((glm::dot(n, p) + d1) * (glm::dot(n, p) + d2) > 0.f)
		return false;

	// xy-plane projection-overlap
	auto xym = (n.z < 0.f ? -1.f : 1.f);
	auto ne0xy = glm::vec4(-e0.y, e0.x, 0.f, 0.f) * xym;
	auto ne1xy = glm::vec4(-e1.y, e1.x, 0.f, 0.f) * xym;
	auto ne2xy = glm::vec4(-e2.y, e2.x, 0.f, 0.f) * xym;

	auto v0xy = glm::vec4(v0.x, v0.y, 0.f, 0.f);
	auto v1xy = glm::vec4(v1.x, v1.y, 0.f, 0.f);
	auto v2xy = glm::vec4(v2.x, v2.y, 0.f, 0.f);

	float de0xy = -glm::dot(ne0xy, v0xy) + glm::max(0.f, dp.x * ne0xy.x) + glm::max(0.f, dp.y * ne0xy.y);
	float de1xy = -glm::dot(ne1xy, v1xy) + glm::max(0.f, dp.x * ne1xy.x) + glm::max(0.f, dp.y * ne1xy.y);
	float de2xy = -glm::dot(ne2xy, v2xy) + glm::max(0.f, dp.x * ne2xy.x) + glm::max(0.f, dp.y * ne2xy.y);

	auto pxy = glm::vec4(p.x, p.y, 0.f, 0.f);

	if ((glm::dot(ne0xy, pxy) + de0xy) < 0.f || (glm::dot(ne1xy, pxy) + de1xy) < 0.f || (glm::dot(ne2xy, pxy) + de2xy) < 0.f)
		return false;

	// yz-plane projection overlap
	auto yzm = (n.x < 0.f ? -1.f : 1.f);
	auto ne0yz = glm::vec4(-e0.z, e0.y, 0.f, 0.f) * yzm;
	auto ne1yz = glm::vec4(-e1.z, e1.y, 0.f, 0.f) * yzm;
	auto ne2yz = glm::vec4(-e2.z, e2.y, 0.f, 0.f) * yzm;

	auto v0yz = glm::vec4(v0.y, v0.z, 0.f, 0.f);
	auto v1yz = glm::vec4(v1.y, v1.z, 0.f, 0.f);
	auto v2yz = glm::vec4(v2.y, v2.z, 0.f, 0.f);

	float de0yz = -glm::dot(ne0yz, v0yz) + glm::max(0.f, dp.y * ne0yz.x) + glm::max(0.f, dp.z * ne0yz.y);
	float de1yz = -glm::dot(ne1yz, v1yz) + glm::max(0.f, dp.y * ne1yz.x) + glm::max(0.f, dp.z * ne1yz.y);
	float de2yz = -glm::dot(ne2yz, v2yz) + glm::max(0.f, dp.y * ne2yz.x) + glm::max(0.f, dp.z * ne2yz.y);

	auto pyz = glm::vec4(p.y, p.z, 0.f, 0.f);

	if ((glm::dot(ne0yz, pyz) + de0yz) < 0.f || (glm::dot(ne1yz, pyz) + de1yz) < 0.f || (glm::dot(ne2yz, pyz) + de2yz) < 0.f)
		return false;

	// zx-plane projection overlap
	auto zxm = (n.y < 0.f ? -1.f : 1.f);
	auto ne0zx = glm::vec4(-e0.x, e0.z, 0.f, 0.f) * zxm;
	auto ne1zx = glm::vec4(-e1.x, e1.z, 0.f, 0.f) * zxm;
	auto ne2zx = glm::vec4(-e2.x, e2.z, 0.f, 0.f) * zxm;

	auto v0zx = glm::vec4(v0.z, v0.x, 0.f, 0.f);
	auto v1zx = glm::vec4(v1.z, v1.x, 0.f, 0.f);
	auto v2zx = glm::vec4(v2.z, v2.x, 0.f, 0.f);

	float de0zx = -glm::dot(ne0zx, v0zx) + glm::max(0.f, dp.y * ne0zx.x) + glm::max(0.f, dp.z * ne0zx.y);
	float de1zx = -glm::dot(ne1zx, v1zx) + glm::max(0.f, dp.y * ne1zx.x) + glm::max(0.f, dp.z * ne1zx.y);
	float de2zx = -glm::dot(ne2zx, v2zx) + glm::max(0.f, dp.y * ne2zx.x) + glm::max(0.f, dp.z * ne2zx.y);

	auto pzx = glm::vec4(p.z, p.x, 0.f, 0.f);

	if ((glm::dot(ne0zx, pzx) + de0zx) < 0.f || (glm::dot(ne1zx, pzx) + de1zx) < 0.f || (glm::dot(ne2zx, pzx) + de2zx) < 0.f)
		return false;

	return true;
}

KDNode* CreateNode(vector<Vertex*> allVerts, IndexObjList indices, int depth, int maxDepth)
{
	// find max and min for spatial median
	glm::vec3 min = glm::vec3(FLT_MAX), max = -min;
	for (auto& i : indices)
	{
		Vertex* v = allVerts[i.second];
		min = glm::min(v->Position, min);
		max = glm::max(v->Position, max);
	}

	KDNode* newNode = new KDNode();
	newNode->min = min;
	newNode->max = max;
	newNode->center = (max + min) / 2.f;
	newNode->halfextents = (max - min) / 2.f;

	// if less than 64 tri's, this is a leaf node. Add indices to it and return it.
	if (depth >= maxDepth || indices.size() < 64 * 3)
	{
		newNode->indices.insert(newNode->indices.end(), indices.begin(), indices.end());
		return newNode;
	}

	// choose the partition plane; 0 = xz, 1 = yz, 2 = xy
	int plane = depth % 3;

	glm::vec3 leftMin = min, rightMin = min;
	glm::vec3 leftMax = max, rightMax = max;

	// get partition plane and split verts
	switch (plane)
	{
	case 0: // yz
		leftMax.x = newNode->center.x;
		rightMin.x = newNode->center.x;
		break;
	case 1: // xz
		leftMax.y = newNode->center.y;
		rightMin.y = newNode->center.y;
		break;
	case 2: // xy
		leftMax.z = newNode->center.z;
		rightMin.z = newNode->center.z;
		break;
	}

	IndexObjList leftIndices, rightIndices;

	// loop through tris, so every 3 indices
	for (unsigned int i = 0; i < indices.size(); i += 3)
	{
		Vertex* tri[3]
		{ 
			allVerts[indices[i + 0].second],
			allVerts[indices[i + 1].second],
			allVerts[indices[i + 2].second]
		};
		if (TriOverlapsAABB(glm::vec4(leftMin, 1), glm::vec4(leftMax, 1), tri))
		{
			leftIndices.push_back(indices[i + 0]);
			leftIndices.push_back(indices[i + 1]);
			leftIndices.push_back(indices[i + 2]);
		}
		if (TriOverlapsAABB(glm::vec4(rightMin, 1), glm::vec4(rightMax, 1), tri))
		{
			rightIndices.push_back(indices[i + 0]);
			rightIndices.push_back(indices[i + 1]);
			rightIndices.push_back(indices[i + 2]);
		}
	}

	newNode->left = leftIndices.empty() ? nullptr : CreateNode(allVerts, leftIndices, depth + 1, maxDepth);
	newNode->right = rightIndices.empty() ? nullptr : CreateNode(allVerts, rightIndices, depth + 1, maxDepth);

	return newNode;
}