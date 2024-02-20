#pragma once

#include <GL/glew.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace AB
{
    class Shader
    {
    public:
        // constructor to read and build the shader
        Shader() = default;
        Shader(const char* vertexPath, const char* fragmentPath);

        // use/activate the shader
        void use();

        // utility uniform functions
        void SetBool(const std::string& name, bool value) const;
        void SetInt(const std::string& name, int value) const;
        void SetUint(const std::string& name, unsigned int value) const;
        void SetFloat(const std::string& name, float value) const;
        void SetVector2(const std::string& name, glm::vec2 value) const;
        void SetVector3(const std::string& name, glm::vec3 value) const;
        void SetVector4(const std::string& name, glm::vec4 value) const;
        void SetMatrix4x4(const std::string& name, glm::mat4x4 value) const;

        unsigned int ID;
    };
}