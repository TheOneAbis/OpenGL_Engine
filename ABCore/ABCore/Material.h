#pragma once

#include <glm/glm.hpp>

namespace AB
{
	class Material
	{
	public:
		glm::vec3 albedo;
		float specular = 0.f;
		float metallic = 0.f;

		Material() = default;

		~Material() = default;
	};
}