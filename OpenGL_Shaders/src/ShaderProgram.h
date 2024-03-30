#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <GL/glew.h>
#include <string>
#include <fstream>
#include <iostream>
#include "ShaderClass.h"
using namespace std;

class ShaderProgram
{
public:
	unsigned int id;
public:
	ShaderProgram(void);
	~ShaderProgram(void);
	void create();
	void link(ShaderClass shader);
	
	void use();

	//GLuint getAttributeLoc(const GLcharARB* name); // this is called beforre glBegin() ..
	//void setAttribute4f(const GLuint loc, const float* floatPtr); // this is called between glBegin() and glEnd()..
	//void setAttribute4i(const GLuint loc, const int* intPtr); // this is called between glBegin() and glEnd()..
	//void setAttributeMat4(const GLuint loc, const float* floatPtr); // must not trust this func !!!!


	//functions to set uniform variables - after use this shader program 
	void setFloat(const char* name, float value);
	void setInt(const char* name, int value);
	void setFloat1V(const char* name, unsigned int count, const float* floatPtr);
	void setFloat3V(const char* name, unsigned int count, const float* floatPtr);
	void setMatrix4fv(const char* name, unsigned int count, const float* floatPtr);
	// ... feel free to add more functions to enable the setting of other data types

	// setup shader sampler texturre data 
	void createDataTexture(unsigned  int& pGLTexID, float* pData, unsigned int pMaxWidth, unsigned int pMaxHeight);

	void setSampler(const char* sampleName, GLuint textureUnit);


private:
	void printProgramInfoLog(unsigned int prog_id);

};

#endif