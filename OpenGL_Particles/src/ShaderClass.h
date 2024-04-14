#ifndef SHADERCLASS_H
#define SHADERCLASS_H

#include <GL/glew.h>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

class ShaderClass
{
public:
	unsigned int id;
	//unsigned int vShader, fShader, cShader;
	//GLuint curAttributeLoc;
public:
	ShaderClass(void);
	~ShaderClass(void);
	void create(const char* shaderFileName, GLenum targetType);
	void createFromContent(const char* sourceContent, GLenum targetType);
	void destroy(); 

private:
	char* loadShaderFile(const char* fn); // load the shader text file
	void printShaderInfoLog(unsigned int shader_id); // print the shader info log to the console

};

#endif