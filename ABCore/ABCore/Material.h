#pragma once

#include <glm/glm.hpp>

namespace AB
{
	class Material
	{
	public:
		glm::vec3 albedo = {1, 1, 1};
		float roughness = 1.f;
		float metallic = 0.f;
		float emissive = 0.f;
		float transmissive = 0.f;

		Material() = default;

		~Material() = default;
	};
}