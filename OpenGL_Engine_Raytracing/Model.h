#pragma once

#include "Mesh.h"
#include <assimp/scene.h>

#include "Transform.h"
#include "Material.h"

class Model
{
public:

	Model() = default;

	Model(const char* path, Transform tm = Transform()) { LoadModel(path); transform = tm; }

	Model(std::vector<Mesh> meshes, Transform tm = Transform());

	void Draw(Shader& shader);

	Transform GetWorldTM();

	void SetWorldTM(Transform newT);
	void SetWorldTM(glm::vec3 translation, glm::quat rotation, glm::vec3 scale);

	Material& GetMaterial();

private:

	// model data
	std::vector<Mesh> meshes;
	std::string directory;
	std::vector<Texture> texturesLoaded;
	Transform transform;
	Material material;

	void LoadModel(std::string path);
	void ProcessNode(aiNode* node, const aiScene* scene);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};