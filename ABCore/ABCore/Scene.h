#pragma once

#include <vector>
#include "GameObject.h"

namespace AB
{
	struct KDNode;

	struct RaycastHit
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texcoord;
		float distance;
		GameObject* gameObject;
	};

	// Singleton to manage all game objects in the world
	class Scene
	{
	public:
		static Scene& Get()
		{
			if (!instance)
				instance = new Scene();
			return *instance;
		}

		Scene(Scene const&) = delete;
		void operator=(Scene const&) = delete;

		// Adds a new game object to the scene.
		GameObject* Add(GameObject&& obj);

		// Returns all game objects in the scene in the order they were added.
		std::vector<GameObject>& GetAllObjects();

		// Finds a particular game object by its name.
		GameObject* Find(std::string objName);

		// Creates the KD tree from the vertex info
		// MAKE SURE ALL VERTS ARE IN WORLD SPACE BEFORE INVOKING THIS
		void CreateTree(int depth);

		// Casts a ray into the scene and retuns if something was hit.
		// MAKE SURE ALL VERTS ARE IN WORLD SPACE BEFORE INVOKING THIS
		bool Raycast(glm::vec3 origin, glm::vec3 dir, RaycastHit* hit = nullptr, float maxDistance = 99999999.f);

		// Draws all objects in the scene via rasterization.
		void Render(Shader& shader);

		~Scene();

	private:

		static Scene* instance;
		Scene() { root = nullptr; };

		bool RaycastInternal(glm::vec3 origin, glm::vec3 dir, RaycastHit* hit, glm::vec3& resultUVW, Vertex(&hitTri)[3], bool& front, MeshType& lastType, float& hitRadius);
		bool RaycastTreeInternal(KDNode* node, glm::vec3 origin, glm::vec3 dir, RaycastHit* hit, glm::vec3& resultUVW, Vertex (&hitTri)[3], bool& front);

		std::vector<GameObject> gameobjects;
		KDNode* root;
		std::vector<Vertex*> allVerts;
	};
}
