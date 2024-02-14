#pragma once

#include "Mesh.h"
#include "Transform.h"
#include "Material.h"

class GameObject
{
public:

	GameObject() = default;
	GameObject(const char* path, Transform tm = Transform());
	GameObject(std::vector<Mesh> meshes, Transform tm = Transform());

	void Draw(Shader& shader);

	Transform GetWorldTM();
	Transform GetLocalTM();

	void SetWorldTM(Transform newT);
	void SetWorldTM(glm::vec3 translation, glm::quat rotation, glm::vec3 scale);
	void SetLocalTM(Transform newT);
	void SetLocalTM(glm::vec3 translation, glm::quat rotation, glm::vec3 scale);

	Material& GetMaterial();

	std::vector<GameObject> GetChildren();
	std::vector<GameObject> GetDescendants();
	GameObject& GetChild(int index);

private:

	// model data
	std::vector<Mesh> meshes;
	std::vector<Texture> texturesLoaded;
	Transform transform;
	std::vector<GameObject> children;
	Material material;

	void LoadModel(std::string path);
};