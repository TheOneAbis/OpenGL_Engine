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
		glm::vec3 emissive = {};
		float transmissive = 0.f;
		float diffuse = 1.f;
		float reflectance = 0.f;
		float refraction = 1.f;

		Material() = default;

		~Material() = default;
	};
}