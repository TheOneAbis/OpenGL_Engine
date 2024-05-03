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

	// test ray vs AABB (using very cool branchless algorithm from https://tavianator.com/2015/ray_box_nan.html)
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

bool PlaneOverlapsAABB(glm::vec3 normal, glm::vec3 vert, glm::vec3 maxbox)
{
	glm::vec3 vmin, vmax;
	for (int axis = 0; axis <= 2; axis++)
	{
		float v = vert[axis];
		if (normal[axis] > 0.f)
		{
			vmin[axis] = -maxbox[axis] - v;
			vmax[axis] = maxbox[axis] - v;
		}
		else
		{
			vmin[axis] = maxbox[axis] - v;
			vmax[axis] = -maxbox[axis] - v;
		}
	}
	if (glm::dot(normal, vmin) > 0.f) return false;
	if (glm::dot(normal, vmax) >= 0.f) return true;
	return false;
}

// Code adapted from https://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/code/tribox3.txt
// Of the 5 different algorithms I tried online, this was the only one that actually worked for some reason
// probably because it's actually based off of the paper more
bool TriOverlapsAABB(glm::vec3 boxMin, glm::vec3 boxMax, Vertex* triVerts[3])
{
	glm::vec3 boxcenter = (boxMin + boxMax) / 2.f;
	glm::vec3 halfextents = (boxMax - boxMin) / 2.f;

	glm::vec3 v0 = triVerts[0]->Position - boxcenter;
	glm::vec3 v1 = triVerts[1]->Position - boxcenter;
	glm::vec3 v2 = triVerts[2]->Position - boxcenter;

	glm::vec3 e0 = v1 - v0;
	glm::vec3 e1 = v2 - v1;
	glm::vec3 e2 = v0 - v2;

	glm::vec3 triMin = glm::min(glm::min(v0, v1), v2);
	glm::vec3 triMax = glm::max(glm::max(v0, v1), v2);
	
	float min, max;

	auto AxisTest_X01 = [&](float a, float b, float fa, float fb) -> bool
		{
			float p0 = a * v0.y - b * v0.z;
			float p2 = a * v2.y - b * v2.z;
			float rad = fa * halfextents.y + fb * halfextents.z;
			return glm::min(p0, p2) <= rad && glm::max(p0, p2) >= -rad;
		};
	auto AxisTest_X2 = [&](float a, float b, float fa, float fb) -> bool
		{
			float p0 = a * v0.y - b * v0.z;
			float p1 = a * v1.y - b * v1.z;
			float rad = fa * halfextents.y + fb * halfextents.z;
			return glm::min(p0, p1) <= rad && glm::max(p0, p1) >= -rad;
		};
	auto AxisTest_Y02 = [&](float a, float b, float fa, float fb) -> bool
		{
			float p0 = -a * v0.x + b * v0.z;
			float p2 = -a * v2.x + b * v2.z;
			float rad = fa * halfextents.x + fb * halfextents.z;
			return glm::min(p0, p2) <= rad && glm::max(p0, p2) >= -rad;
		};
	auto AxisTest_Y1 = [&](float a, float b, float fa, float fb) -> bool
		{
			float p0 = -a * v0.x + b * v0.z;
			float p1 = -a * v1.x + b * v1.z;
			float rad = fa * halfextents.x + fb * halfextents.z;
			return glm::min(p0, p1) <= rad && glm::max(p0, p1) >= -rad;
		};
	auto AxisTest_Z12 = [&](float a, float b, float fa, float fb) -> bool
		{
			float p1 = a * v1.x - b * v1.y;
			float p2 = a * v2.x - b * v2.y;
			float rad = fa * halfextents.x + fb * halfextents.y;
			return glm::min(p1, p2) <= rad && glm::max(p1, p2) >= -rad;
		};
	auto AxisTest_Z0 = [&](float a, float b, float fa, float fb) -> bool
		{
			float p0 = a * v0.x - b * v0.y;
			float p1 = a * v1.x - b * v1.y;
			float rad = fa * halfextents.x + fb * halfextents.y;
			return glm::min(p0, p1) <= rad && glm::max(p0, p1) >= -rad;
		};

	if (!AxisTest_X01(e0.z, e0.y, glm::abs(e0.z), glm::abs(e0.y))) return false;
	if (!AxisTest_Y02(e0.z, e0.x, glm::abs(e0.z), glm::abs(e0.x))) return false;
	if (!AxisTest_Z12(e0.y, e0.x, glm::abs(e0.y), glm::abs(e0.x))) return false;

	if (!AxisTest_X01(e1.z, e1.y, glm::abs(e1.z), glm::abs(e1.y))) return false;
	if (!AxisTest_Y02(e1.z, e1.x, glm::abs(e1.z), glm::abs(e1.x))) return false;
	if (!AxisTest_Z0(e1.y, e1.x, glm::abs(e1.y), glm::abs(e1.x))) return false;

	if (!AxisTest_X2(e2.z, e2.y, glm::abs(e2.z), glm::abs(e2.y))) return false;
	if (!AxisTest_Y1(e2.z, e2.x, glm::abs(e2.z), glm::abs(e2.x))) return false;
	if (!AxisTest_Z12(e2.y, e2.x, glm::abs(e2.y), glm::abs(e2.x))) return false;

	// test in xyz directions
	for (int i = 0; i < 3; i++)
		if (triMin[i] > halfextents[i] || triMax[i] < -halfextents[i]) return false;

	// test against normal
	return PlaneOverlapsAABB(glm::cross(e0, e1), v0, halfextents);
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
		if (TriOverlapsAABB(leftMin, leftMax, tri))
		{
			leftIndices.push_back(indices[i + 0]);
			leftIndices.push_back(indices[i + 1]);
			leftIndices.push_back(indices[i + 2]);
		}
		if (TriOverlapsAABB(rightMin, rightMax, tri))
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