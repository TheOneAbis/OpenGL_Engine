#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include "Shader.h"

namespace AB
{
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoord;
	};

	struct Texture
	{
		unsigned int id;
		std::string type;
		std::string path;
	};

	enum MeshType
	{
		MESH_SPHERE,
		MESH_TRI
	};

	class Mesh
	{
	public:

		// mesh data
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<Texture> textures;

		Mesh() { VAO = VBO = EBO = 0; };
		~Mesh();
		Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
		Mesh(float radius);

		Texture& AddTexture(std::string typeName, const char* path, const std::string& directory);
		void RefreshBuffers();
		void Draw(Shader& shader, int drawMode = 0x0004);

		MeshType type = MESH_SPHERE;
		float radius;

	private:

		unsigned int VAO, VBO, EBO;
	};
}