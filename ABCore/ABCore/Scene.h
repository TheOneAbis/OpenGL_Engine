#pragma once

#include <vector>
#include "GameObject.h"

namespace AB
{
	struct KDNode;

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

		GameObject* Add(GameObject&& obj);
		std::vector<GameObject>& GetAllObjects();
		GameObject* Find(std::string objName);

		void CreateTree(int depth);

		void Render(Shader& shader);

		~Scene();

	private:

		static Scene* instance;
		Scene() {};

		std::vector<GameObject> gameobjects;
		KDNode* root;
	};
}
