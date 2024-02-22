#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
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

	class Mesh
	{
	public:

		// mesh data
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<Texture> textures;

		Mesh() = default;
		Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

		void Draw(Shader& shader, int drawMode = GL_TRIANGLES);

	private:

		unsigned int VAO, VBO, EBO;

		void SetupMesh();
	};
}