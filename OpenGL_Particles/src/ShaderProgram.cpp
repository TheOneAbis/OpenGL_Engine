#include "ShaderProgram.h"
#include <iostream>
#include <GL/glew.h>
using namespace std;
ShaderProgram::ShaderProgram(void)
{
	id = 0;
}

ShaderProgram::~ShaderProgram(void)
{

}

void ShaderProgram::create()
{
	id = glCreateProgram();
}

void ShaderProgram::link(ShaderClass shader)
{
	glAttachShader(id, shader.id);
	glLinkProgram(id);

	int status;
	glGetProgramiv(id, GL_LINK_STATUS, &status);
	if (status != GL_TRUE) 
	{
		printProgramInfoLog(id);
		return;
	}
}


//GLuint ShaderProgram::getAttributeLoc(const GLcharARB* name)
//{
//	return glGetAttribLocationARB(id, name);
//}
//void ShaderProgram::setAttribute4f(const GLuint loc, const float* floatPtr)
//{
//	if (loc != -1)
//		glVertexAttrib4fvARB(loc, floatPtr);
//	else
//		cout << "can not set 4f attribute, loc id is -1 !" << endl;
//}
//void ShaderProgram::setAttribute4i(const GLuint loc, const int* intPtr)
//{
//	if (loc != -1) {
//		glVertexAttrib4ivARB(loc, intPtr);
//	}
//	else
//		cout << "can not set 4i attribute, loc id is -1 !" << endl;
//}
//void ShaderProgram::setAttributeMat4(const GLuint loc, const float* floatPtr)
//{
//	if (loc != -1) {
//		GLuint i;
//		for (i = loc; i < loc + 4; i++) { // set each column .. 
//			glVertexAttrib4fARB(i, floatPtr[(i - loc) * 4 + 0], floatPtr[(i - loc) * 4 + 1], floatPtr[(i - loc) * 4 + 2], floatPtr[(i - loc) * 4 + 3]);
//		}
//	}
//	//else 
//		//cout<<"can not set matrix attribute, loc id is -1 !"<<endl;
//}


void ShaderProgram::setInt(const char* name, int value)
{
	unsigned int loc = glGetUniformLocation(id, name);
	if (loc == -1) {
		cout << "Uniform integer: " << name <<"doesn't exist."<< endl;
	}
	else {
		glUniform1i(loc, value);
	}
}


void ShaderProgram::setFloat(const char* name, float value)
{
	unsigned int loc = glGetUniformLocation(id, name);
	if (loc == -1) {
		cout << "Uniform float: " << name <<" doesn't exist."<< endl;
	}
	else {
		glUniform1f(loc, value);
	}
}
void ShaderProgram::setFloat1V(const char* name, unsigned int count, const float* floatPtr)
{
	unsigned int loc = glGetUniformLocation(id, name);
	if (loc == -1) {
		cout << "Uniform float1 vector: " << name << " doesn't exist."<<endl;
	}
	else {
		glUniform1fv(loc, count, floatPtr);
	}
}

void ShaderProgram::setFloat3V(const char* name, unsigned int count, const float* floatPtr)
{
	unsigned int loc = glGetUniformLocation(id, name);
	if (loc == -1) {
		cout << "Uniform float3 vector: " << name << " doesn't exist." << endl;
	}
	else {
		glUniform3fv(loc, count, floatPtr);
	}
}

void ShaderProgram::setMatrix4fv(const char* name, unsigned int count, const float* floatPtr)
{
	unsigned int loc = glGetUniformLocation(id, name);
	if (loc == -1) {
		cout << "Uniform matrix: " << name << " doesn't exist."<<endl;
	}
	else {
		glUniformMatrix4fv(loc, count, GL_FALSE, floatPtr);
	}
}

void ShaderProgram::printProgramInfoLog (unsigned int prog_id)
{
	int infoLogLen = 0;
	int charsWritten = 0;
	char* infoLog;

	glGetProgramiv(prog_id, GL_INFO_LOG_LENGTH, &infoLogLen);

	if (infoLogLen > 0)
	{
		infoLog = new char [infoLogLen];
		glGetProgramInfoLog(prog_id, infoLogLen, &charsWritten, infoLog);
		cout << infoLog << endl;
		delete [] infoLog;
	}
}

// setup shader sampler texture data 
void ShaderProgram::createDataTexture(unsigned int& pGLTexID, float* pData, unsigned int pMaxWidth, unsigned int pMaxHeight)
{

	glGenTextures(1, &pGLTexID);
	glActiveTexture(GL_TEXTURE0 + pGLTexID);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, pGLTexID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA_FLOAT32_ATI/*GL_RGBA32F_ARB*/, pMaxWidth, pMaxHeight, 0, GL_RGBA, GL_FLOAT, pData );

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F/*GL_RGBA32F_ARB*/, pMaxWidth, pMaxHeight, 0, GL_RGBA, GL_FLOAT, pData);
	glDisable(GL_TEXTURE_2D);
}

void ShaderProgram::setSampler(const char* sampleName, GLuint textureUnit)
{
	GLint samplerLocation = glGetUniformLocation(id, sampleName);
	glUniform1i(samplerLocation, textureUnit);
}

