#include "Transform.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace AB;

Transform::Transform()
{
	this->translation = glm::vec3();
	this->rotation = glm::quat();
	this->scale = glm::vec3(1.f, 1.f, 1.f);

	// mark the stored matrix as needing to be fixed later
	dirty = true;
}

Transform::Transform(glm::vec3 translation, glm::quat rotation, glm::vec3 scale)
{
	this->translation = translation;
	this->rotation = rotation;
	this->scale = scale;

	// mark the stored matrix as needing to be fixed later
	dirty = true;
}

Transform::Transform(glm::mat4 matrix)
{
	// extract translation, rotation, and scale
	this->translation = glm::vec3(matrix[3]);

	float scaleX = glm::length(matrix[0]);
	float scaleY = glm::length(matrix[1]);
	float scaleZ = glm::length(matrix[2]);
	this->scale = glm::vec3(scaleX, scaleY, scaleZ);

	glm::mat3 rotmat(matrix);
	rotmat[0] /= scaleX;
	rotmat[1] /= scaleY;
	rotmat[2] /= scaleZ;
	this->rotation = glm::quat(rotmat);

	// store the matrix (meaning we won't need to fix it later)
	this->matrix = matrix;
	dirty = false;
}

glm::vec3 Transform::GetTranslation()
{
	return translation;
}
void Transform::Translate(glm::vec3 translationOffset)
{
	translation += translationOffset;
	dirty = true;
}
glm::quat Transform::GetRotation()
{
	return rotation;
}
glm::vec3 Transform::GetScale()
{
	return scale;
}

glm::mat4 Transform::GetMatrix()
{
	UpdateMatrix();
	return matrix;
}
inline void Transform::UpdateMatrix()
{
	if (dirty)
	{
		matrix = glm::scale(glm::translate(glm::mat4(), translation) * glm::toMat4(rotation), scale);
		dirty = false;
	}
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
	dirty = true;
}
void Transform::SetRotation(glm::quat newRotation)
{
	rotation = glm::normalize(newRotation);
	dirty = true;
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
	dirty = true;
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
	dirty = true;
}
void Transform::SetScale(glm::vec3 newScale)
{
	scale = newScale;
	dirty = true;
}