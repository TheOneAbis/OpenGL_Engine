#pragma once

#include "Mesh.h"
#include "Transform.h"
#include "Material.h"
#include "Shader.h"

namespace AB
{
	class GameObject
	{
	public:

		GameObject();
		GameObject(GameObject&& other);

		GameObject(const char* path, std::string name = "Game Object", Transform tm = Transform(), GameObject* parent = nullptr);
		GameObject(std::vector<Mesh> meshes, std::string name = "Game Object", Transform tm = Transform(), GameObject* parent = nullptr);
		~GameObject();

		void Draw(Shader& shader, int drawMode = GL_TRIANGLES);

		Transform GetWorldTM();
		Transform GetLocalTM();

		void SetWorldTM(Transform newT);
		void SetWorldTM(glm::vec3 translation, glm::quat rotation, glm::vec3 scale);
		void SetLocalTM(Transform newT);
		void SetLocalTM(glm::vec3 translation, glm::quat rotation, glm::vec3 scale);

		Material& GetMaterial();
		std::vector<Mesh>& GetMeshes();

		std::vector<GameObject*> GetChildren();
		std::vector<GameObject*> GetDescendants();
		GameObject* GetChild(int index);
		GameObject* GetParent();

		std::string GetName();

	private:

		std::vector<Mesh> meshes;
		Transform localTm, worldTm;
		Material material;

		std::vector<GameObject*> children;
		GameObject* parent;

		std::string name;
	};
}
