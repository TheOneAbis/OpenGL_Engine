#pragma once

#include "Mesh.h"
#include <assimp/scene.h>

class Model
{
public:

	Model() = default;

	Model(const char* path) { LoadModel(path); }

	void Draw(Shader& shader);
private:

	// model data
	std::vector<Mesh> meshes;
	std::string directory;
	std::vector<Texture> texturesLoaded;

	void LoadModel(std::string path);
	void ProcessNode(aiNode* node, const aiScene* scene);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};