#include "Scene.h"

using namespace AB;
using namespace std;

Scene* Scene::instance;

struct AB::KDNode
{
	// AABB
	glm::vec3 min, max;

	// indicies of tris inside AABB
	vector<unsigned int> vertIndices;

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
		// TODO: SAT TEST
	}

	newNode->left = CreateNode(allVerts, leftIndices, depth + 1, maxDepth, leftMin, leftMax);
	newNode->right = CreateNode(allVerts, rightIndices, depth + 1, maxDepth, rightMin, rightMax);

}