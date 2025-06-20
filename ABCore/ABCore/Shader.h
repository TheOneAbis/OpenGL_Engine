#pragma once

#include <string>

namespace glm
{
    enum precision;
    namespace detail
    {
        template <typename T, precision P>
        struct tvec2;
        template <typename T, precision P>
        struct tvec3;
        template <typename T, precision P>
        struct tvec4;
        template <typename T, precision P>
        struct tmat4x4;
    }
    typedef detail::tvec2<float, (precision)0> vec2;
    typedef detail::tvec3<float, (precision)0> vec3;
    typedef detail::tvec4<float, (precision)0> vec4;
    typedef detail::tmat4x4<float, (precision)0> mat4x4;
}

namespace AB
{
    class Shader
    {
    public:
        // constructor to read and build the shader
        Shader() = default;
        Shader(const char* vertexPath, const char* fragmentPath);
        Shader(const char* computePath);

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