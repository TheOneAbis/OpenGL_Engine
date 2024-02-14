#include "Transform.h"

Transform::Transform()
{
	this->translation = glm::vec3();
	this->rotation = glm::quat();
	this->scale = glm::vec3(1.f, 1.f, 1.f);
}

Transform::Transform(glm::vec3 translation, glm::quat rotation, glm::vec3 scale)
{
	this->translation = translation;
	this->rotation = rotation;
	this->scale = scale;
}

glm::vec3 Transform::GetTranslation()
{
	return translation;
}
void Transform::Translate(glm::vec3 translationOffset)
{
	translation += translationOffset;
}
glm::quat Transform::GetRotation()
{
	return rotation;
}
glm::vec3 Transform::GetScale()
{
	return scale;
}

glm::vec3 Transform::GetForward()
{
	return glm::vec3(0, 0, 1) * rotation;
}
glm::vec3 Transform::GetUp()
{
	return glm::vec3(0, 1, 0) * rotation;
}
glm::vec3 Transform::GetRight()
{
	return glm::vec3(1, 0, 0) * rotation;
}

void Transform::SetTranslation(glm::vec3 newTranslation)
{
	translation = newTranslation;
}
void Transform::SetRotation(glm::quat newRotation)
{
	rotation = glm::normalize(newRotation);
}
void Transform::SetRotationEulersXYZ(glm::vec3 eulers)
{
	glm::vec3 c = glm::cos(eulers * 0.5f);
	glm::vec3 s = glm::sin(eulers * 0.5f);
	rotation =
	{
		c.x * c.y * c.z + s.x * s.y * s.z,
		s.x * c.y * c.z - c.x * s.y * s.z,
		c.x * s.y * c.z + s.x * c.y * s.z,
		c.x * c.y * s.z - s.x * s.y * c.z
	};
}
void Transform::SetRotationEulersZYX(glm::vec3 eulers)
{
	glm::vec3 c = glm::cos(eulers * 0.5f);
	glm::vec3 s = glm::sin(eulers * 0.5f);
	rotation =
	{
		c.x * c.y * c.z - s.x * s.y * s.z,
		s.x * c.y * c.z + c.x * s.y * s.z,
		c.x * s.y * c.z - s.x * c.y * s.z,
		c.x * c.y * s.z + s.x * s.y * c.z
	};
}
void Transform::SetScale(glm::vec3 newScale)
{
	scale = newScale;
}