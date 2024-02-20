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

		GameObject() = default;
		GameObject(GameObject* parent = nullptr, const char* path, Transform tm = Transform());
		GameObject(GameObject* parent = nullptr, Mesh mesh, Transform tm = Transform());
		~GameObject() = default;

		void Draw(Shader& shader);

		Transform GetWorldTM();
		Transform GetLocalTM();

		void SetWorldTM(Transform newT);
		void SetWorldTM(glm::vec3 translation, glm::quat rotation, glm::vec3 scale);
		void SetLocalTM(Transform newT);
		void SetLocalTM(glm::vec3 translation, glm::quat rotation, glm::vec3 scale);

		Material& GetMaterial();
		Mesh& GetMesh();

		std::vector<GameObject> GetChildren();
		std::vector<GameObject> GetDescendants();
		GameObject& GetChild(int index);
		GameObject& GetParent();

	private:

		Mesh mesh;
		Transform localTm, worldTm;
		Material material;

		std::vector<GameObject> children;
		GameObject* parent = nullptr;

		void LoadModel(std::string path);
	};
}
