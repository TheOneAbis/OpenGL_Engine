#pragma once

#include <glm/glm.hpp>

struct Light
{
	glm::vec3 position;
	glm::vec3 ambientColor;
	glm::vec3 diffuseColor;
	glm::vec3 specularColor;
	float range;
	float specularExp;
};