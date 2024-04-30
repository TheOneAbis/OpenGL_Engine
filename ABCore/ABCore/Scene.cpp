#include "Scene.h"

#include <iostream>

#define EPSILON 0.0001f

using namespace AB;
using namespace std;

KDNode* CreateNode(vector<Vertex>* allVerts, vector<unsigned int> indices, int depth, int maxDepth, glm::vec3 min, glm::vec3 max);

Scene* Scene::instance;

struct AB::KDNode
{
	// AABB
	glm::vec3 min, max;

	// pointers to verts inside AABB
	// every 3 verts = a tri. Thus, there should be duplicate ptrs in here.
	vector<unsigned int> indices;

	// children
	KDNode* left;
	KDNode* right;

	~KDNode() { delete left; delete right; };
};

Scene::~Scene()
{
	delete instance;
	delete root;
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

	return root ? 
		RaycastTreeInternal(origin, dir, hit, maxDistance) : 
		RaycastInternal(origin, dir, hit, maxDistance);
}

bool Scene::RaycastInternal(glm::vec3 origin, glm::vec3 dir, RaycastHit* hit, float maxDistance)
{
	bool successful = false;
	glm::vec3 resultUVW{ -1, -1, maxDistance };
	Vertex hitTri[3];
	bool front = true;

	MeshType lastType;
	float hitRadius;

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

					lastType = MESH_SPHERE;
					resultUVW.z = w0;

					hitRadius = m.radius;

					hit->gameObject = &obj;
					successful = true;
				}
				break;
			}
		}
	}

	// interpolate bary coords to find pos, normal, and texcoords
	if (successful)
	{
		if (lastType == MESH_TRI)
		{
			hit->position = (1 - resultUVW.x - resultUVW.y) * hitTri[0].Position + resultUVW.x * hitTri[1].Position + resultUVW.y * hitTri[2].Position;
			hit->normal = glm::normalize((1 - resultUVW.x - resultUVW.y) * hitTri[0].Normal + resultUVW.x * hitTri[1].Normal + resultUVW.y * hitTri[2].Normal) * (front ? 1.f : -1.f);
			hit->texcoord = (1 - resultUVW.x - resultUVW.y) * hitTri[0].TexCoord + resultUVW.x * hitTri[1].TexCoord + resultUVW.y * hitTri[2].TexCoord;
		}
		else if (lastType == MESH_SPHERE)
		{
			hit->position = origin + (dir * resultUVW.z);
			hit->normal = (hit->position - hit->gameObject->GetWorldTM().GetTranslation()) / hitRadius;
			if (glm::dot(dir, hit->normal) > 0.f)
				hit->normal *= -1.f;
		}
	}

	return successful;
}

// Note: Due to time constraints KD trees only track primitive tri's, and not spheres.
bool Scene::RaycastTreeInternal(glm::vec3 origin, glm::vec3 dir, RaycastHit* hit, float maxDistance)
{
	bool successful = false;
	glm::vec3 resultUVW{ -1, -1, maxDistance };
	Vertex hitTri[3];
	bool front = true;

	

	return false;
}

vector<unsigned int> GetPrimitives(KDNode& node)
{

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
	vector<Vertex> allVerts;
	vector<unsigned int> allIndices;
	for (auto& obj : gameobjects)
	{
		for (auto& mesh : obj.GetMeshes())
		{
			allVerts.insert(allVerts.end(), mesh.vertices.begin(), mesh.vertices.end());
			allIndices.insert(allIndices.end(), mesh.indices.begin(), mesh.indices.end());
		}
	}
	
	// create the tree with the verts
	glm::vec3 min = glm::vec3(-FLT_MAX), max = glm::vec3(FLT_MAX);
	for (auto& v : allVerts)
	{
		if (v.Position.x < min.x)
			min.x = v.Position.x;
		if (v.Position.y < min.y)
			min.y = v.Position.y;
		if (v.Position.z < min.z)
			min.z = v.Position.z;

		if (v.Position.x > max.x)
			max.x = v.Position.x;
		if (v.Position.y > max.y)
			max.y = v.Position.y;
		if (v.Position.z > max.z)
			max.z = v.Position.z;
	}
	root = CreateNode(&allVerts, allIndices, 0, maxDepth, min, max);
}

KDNode* CreateNode(vector<Vertex>* allVerts, vector<unsigned int> indices, int depth, int maxDepth, glm::vec3 min, glm::vec3 max)
{
	if (depth > maxDepth || indices.empty()) return nullptr;

	KDNode* newNode = new KDNode();
	newNode->min = min;
	newNode->max = max;

	// choose the partition plane; 0 = xz, 1 = yz, 2 = xy
	int plane = depth % 3;
	glm::vec3 center = (max + min) / 2.f;

	glm::vec3 leftMin = min, rightMin = min;
	glm::vec3 leftMax = max, rightMax = max;

	// get partition plane and split verts
	switch (plane)
	{
	case 0: // xz
		leftMax.y = center.y;
		rightMin.y = center.y;
		break;

	case 1: // yz
		leftMax.x = center.x;
		rightMin.x = center.x;
		break;

	case 2: // xy
		leftMax.z = center.z;
		rightMin.z = center.z;
		break;
	}

	vector<unsigned int> leftIndices, rightIndices;
	// loop through tris, so every 3 indices
	for (unsigned int i = 0; i < indices.size(); i += 3)
	{
		glm::vec3 p0 = (*allVerts)[i + 0].Position;
		glm::vec3 p1 = (*allVerts)[i + 1].Position;
		glm::vec3 p2 = (*allVerts)[i + 2].Position;

		// SAT test; if tri is inside partitioned AABB, add it to the list

	}

	newNode->left = CreateNode(allVerts, leftIndices, depth + 1, maxDepth, leftMin, leftMax);
	newNode->right = CreateNode(allVerts, rightIndices, depth + 1, maxDepth, rightMin, rightMax);
}