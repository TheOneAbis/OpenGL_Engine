#include "GameObject.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <glm/gtc/matrix_transform.hpp>

#include "Transform.h"
#include <iostream>

using namespace std;
using namespace AB;

//////////////////////////////////////////////////////////////////////
// 
//	The below helper functions utilize the assimp API to load in    
//	models from an FBX, OBJ, or other 3D model file format.
// 
//////////////////////////////////////////////////////////////////////

static Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, const string directory)
{
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<Texture> textures;

	// process vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;

		// process vertex positions, normals and texture coords
		glm::vec3 vec;
		vec.x = mesh->mVertices[i].x;
		vec.y = mesh->mVertices[i].y;
		vec.z = mesh->mVertices[i].z;
		vertex.Position = vec;

		vec.x = mesh->mNormals[i].x;
		vec.y = mesh->mNormals[i].y;
		vec.z = mesh->mNormals[i].z;
		vertex.Normal = vec;

		if (mesh->mTextureCoords[0]) // does the mesh contain texture coords?
		{
			glm::vec2 texVec;
			texVec.x = mesh->mTextureCoords[0][i].x;
			texVec.y = mesh->mTextureCoords[0][i].y;
			vertex.TexCoord = texVec;
		}
		else
		{
			vertex.TexCoord = glm::vec2(0.f, 0.f);
		}
		vertices.push_back(vertex);
	}

	// process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	//// process material
	//if (mesh->mMaterialIndex >= 0)
	//{
	//	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

	//	// load diffuse maps
	//	vector<Texture> diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
	//	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
	//	
	//	// load specular maps
	//	vector<Texture> specularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
	//	textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	//}

	return Mesh(vertices, indices, textures);
}

static void ProcessNode(vector<Mesh>& meshes, aiNode* node, const aiScene* scene, const string directory)
{
	// process all the node's meshes (if any)
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(ProcessMesh(mesh, scene, directory));
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(meshes, node->mChildren[i], scene, directory);
	}
}

static vector<Mesh> LoadModelMeshes(string path)
{
	vector<Mesh> meshes;
	Assimp::Importer import;
	// if model doesn't entirely consist of tri's, transform them to tri's first
	const aiScene * scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		cout << "ASSIMP ERROR: " << import.GetErrorString() << endl;
		return meshes;
	}

	// Process all nodes and add them as child game objects
	ProcessNode(meshes, scene->mRootNode, scene, path.substr(0, path.find_last_of('/')));
	return meshes;
}


/////////////////////////////////////////////////////
//    ------- GameObject definitions ---------     //
/////////////////////////////////////////////////////
void GameObject::SetParams(vector<Mesh> meshes, string name, Transform localT, GameObject* parent)
{
	this->parent = parent;
	if (parent)
	{
		this->parent->children.push_back(this);
	}

	this->meshes = meshes;
	this->name = name;
	SetLocalTM(localT);
}

GameObject::GameObject()
{
	this->parent = nullptr;
	this->name = "Game Object";
}
GameObject::GameObject(GameObject&& other) noexcept
{
	SetParams(other.meshes, other.name, other.localTm, other.parent);
	this->material = other.material;
}
GameObject::GameObject(const GameObject& other)
{
	SetParams(other.meshes, other.name, other.localTm, other.parent);
	this->material = other.material;
}
void GameObject::operator=(GameObject&& other) noexcept
{
	SetParams(other.meshes, other.name, other.localTm, other.parent);
	this->material = other.material;
}
void GameObject::operator=(const GameObject& other)
{
	SetParams(other.meshes, other.name, other.localTm, other.parent);
	this->material = other.material;
}

GameObject::GameObject(vector<Mesh> meshes, string name, Transform localT, GameObject* parent)
{
	SetParams(meshes, name, localT, parent);
}
GameObject::GameObject(const char* path, string name, Transform localT, GameObject* parent)
{
	SetParams(LoadModelMeshes(path), name, localT, parent);
}

GameObject::~GameObject()
{
	// remove this from its parent's children
	if (parent && !parent->children.empty())
	{
		parent->children.erase(find(parent->children.begin(), parent->children.end(), this));
	}

	// add the children to this parent's children
	for (auto& child : children)
	{
		child->parent = this->parent;
		if (parent && !parent->children.empty())
		{
			parent->children.push_back(child);
		}
	}
}

void GameObject::Draw(Shader& shader, int drawMode)
{
	glm::mat4x4 world = worldTm.GetMatrix();

	shader.SetMatrix4x4("world", world);
	shader.SetMatrix4x4("worldInvTranspose", glm::inverse(glm::transpose(world)));

	shader.SetVector3("albedoColor", material.albedo);
	shader.SetFloat("metallic", material.metallic);
	shader.SetFloat("roughness", material.roughness);

	// draw all meshes on this object
	for (Mesh& mesh : meshes)
	{
		mesh.Draw(shader, drawMode);
	}
}

Transform GameObject::GetWorldTM()
{
	return worldTm;
}
Transform GameObject::GetLocalTM()
{
	return localTm;
}

// world transform
void GameObject::SetWorldTM(Transform newT)
{
	SetLocalTM(parent ? glm::inverse(parent->worldTm.GetMatrix()) * worldTm.GetMatrix() : newT);
}
void GameObject::SetWorldTM(glm::vec3 translation, glm::quat rotation, glm::vec3 scale)
{
	SetWorldTM(Transform(translation, rotation, scale));
}

// local transform
void GameObject::SetLocalTM(Transform newT)
{
	localTm = newT;
	worldTm = parent ? parent->worldTm.GetMatrix() * localTm.GetMatrix() : localTm;

	// tell children to update their world transforms accordingly
	for (auto& child : children)
	{
		child->SetLocalTM(child->GetLocalTM());
	}
}
void GameObject::SetLocalTM(glm::vec3 translation, glm::quat rotation, glm::vec3 scale)
{
	SetLocalTM(Transform(translation, rotation, scale));
}

Material& GameObject::GetMaterial()
{
	return material;
}

vector<GameObject*> GameObject::GetChildren()
{
	return children;
}
vector<GameObject*> GameObject::GetDescendants()
{
	vector<GameObject*> descendants;
	
	// breadth-first
	if (!children.empty())
	{
		descendants.insert(descendants.end(), children.begin(), children.end());
	}

	for (auto& child : children)
	{
		vector<GameObject*> childDescendants = child->GetDescendants();
		if (!childDescendants.empty())
			descendants.insert(descendants.end(), childDescendants.begin(), childDescendants.end());
	}

	return descendants;
}
GameObject* GameObject::GetChild(int index)
{
	if (index < 0 || index >= children.size())
		return nullptr;

	return children[index];
}
GameObject* GameObject::GetParent()
{
	return parent;
}

vector<Mesh>& GameObject::GetMeshes()
{
	return meshes;
}

string GameObject::GetName()
{
	return name;
}

//vector<Texture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
//{
//	vector<Texture> textures;
//	for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
//	{
//		aiString str;
//		mat->GetTexture(type, i, &str);
//
//		bool skip = false;
//		for (unsigned int j = 0; j < texturesLoaded.size(); j++)
//		{
//			if (strcmp(texturesLoaded[j].path.data(), str.C_Str()) == 0)
//			{
//				textures.push_back(texturesLoaded[j]);
//				skip = true;
//				break;
//			}
//		}
//		if (!skip) // if textures hasn't been loaded already, load it
//		{
//			Texture texture;
//			texture.id = TextureFromFile(str.C_Str(), directory);
//			texture.type = typeName;
//			texture.path = str.C_Str();
//			textures.push_back(texture);
//			texturesLoaded.push_back(texture);
//		}
//	}
//
//	return textures;
//}

/*unsigned int TextureFromFile(const char* path, const string& directory, bool gamma)
{
	string filename = string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}*/