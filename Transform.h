#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace AB
{
	class Transform
	{
	public:

		Transform();
		Transform(glm::vec3 translation, glm::quat rotation, glm::vec3 scale);

		~Transform() = default;

		glm::vec3 GetTranslation();
		glm::quat GetRotation();
		glm::vec3 GetScale();

		void SetTranslation(glm::vec3 newTranslation);
		void Translate(glm::vec3 translationOffset);

		void SetRotation(glm::quat newRotation);
		void SetRotationEulersXYZ(glm::vec3 eulers);
		void SetRotationEulersZYX(glm::vec3 eulers);

		void SetScale(glm::vec3 newScale);

		glm::vec3 GetForward();
		glm::vec3 GetUp();
		glm::vec3 GetRight();

	private:

		glm::vec3 translation;
		glm::quat rotation;
		glm::vec3 scale;

	};
}