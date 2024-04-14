#include <GL/glew.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "ParticleSystem.h"

#include <math.h>
#include <iostream>
using namespace std;


// create the particles with the shader storage buffer objects
ParticleSystem::ParticleSystem()
{
	num = 0;
	resolution = vec2();
	spherePos = vec3();
	sphereRadius = 0;

	size_min_point = vec3(0);
	size_max_point = vec3(0);

	pos_array = NULL;
	color_array = NULL;
	
	vao = 0;
	pos_ssbo = color_ssbo = 0; 

	cShaderProg = ShaderProgram();
	vfShaderProg = ShaderProgram();

	cShader = ShaderClass();
	vShader = ShaderClass();
	fShader = ShaderClass();
}

ParticleSystem::~ParticleSystem()
{
	//delete[] pos_array;
	//delete[] dir_array;
	//delete[] speed_array;
	//delete[] color_array;
}

float ParticleSystem::randomf(float min, float max)
{
	if (max < min) return 0.0f;
	float n = ((float)rand()) / ((float)RAND_MAX);
	float range = max - min;
	return n * range + min;
}

void ParticleSystem::create(vec2 particle_resolution, vec3 min_point, vec3 max_point, vec3 origin, vec3 spherePos, float sphereRadius,
	const char* compute_shader_file, const char* vertex_shader_file, const char* fragment_shader_file)
{
	if (particle_resolution.length() <= 0) {
		cout << "The particle system wasn't created!" << endl;
		return;
	}
	num = particle_resolution.x * particle_resolution.y;
	resolution = particle_resolution;
	size_min_point = min_point;
	size_max_point = max_point;
	this->origin = origin;
	this->spherePos = spherePos;
	this->sphereRadius = sphereRadius;

	// ********** load and set up shaders and shader programs **********
	// load the compute shader and link it to a shader program
	cShader.create(compute_shader_file, GL_COMPUTE_SHADER);
	vShader.create(vertex_shader_file, GL_VERTEX_SHADER);
	fShader.create(fragment_shader_file, GL_FRAGMENT_SHADER);

	// create shader programs
	cShaderProg.create();
	vfShaderProg.create();

	// link shaders to a shader prorgam
	// Note: A Compute Shader must be in a shader program all by itself. 
	// There cannot be vertex, fragment, etc.shaders in a shader program with the compute shader.
	vfShaderProg.link(vShader); // link vertex shader 
	vfShaderProg.link(fShader); // link fragment shader
	cShaderProg.link(cShader); // link compute shader 

	// after linking shaders to the shader program, it is safer to destroy the shader as they're no longer needed
	vShader.destroy();
	fShader.destroy();
	cShader.destroy();

	ResetBuffers(resolution);
}

void ParticleSystem::ResetBuffers(uvec2 newResolution)
{
	resolution = newResolution;
	newResolution += uvec2(1, 1);
	num = newResolution.x * newResolution.y;

	// delete old buffers
	glDeleteBuffers(1, &pos_ssbo);
	glDeleteBuffers(1, &color_ssbo);
	glDeleteVertexArrays(1, &vao);

	// create a ssbo of particle positions
	glGenBuffers(1, &pos_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pos_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, num * sizeof(vec4), NULL, GL_STATIC_DRAW); // there isn't data yet, just init memory, data will sent at run-time. 

	// map and create the postion array
	GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	pos_array = (vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, num * sizeof(vec4), bufMask);
	unsigned int index;
	if (pos_array != NULL)
	{
		for (unsigned int j = 0; j <= resolution.y; j++)
		{
			for (unsigned int i = 0; i <= resolution.x; i++)
			{
				index = j * resolution.x + i;
				pos_array[index].x = glm::mix(size_min_point.x, size_max_point.x, (float)i / (float)resolution.x);
				pos_array[index].y = glm::mix(size_min_point.y, size_max_point.y, (float)j / (float)resolution.y);
				pos_array[index].z = glm::mix(size_min_point.z, size_max_point.z, (float)i / (float)resolution.x);
				pos_array[index].w = 1.0f;
			}
		}
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	// map and create the color array
	glGenBuffers(1, &color_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, color_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, num * sizeof(vec4), NULL, GL_STATIC_DRAW);
	color_array = (vec4*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, num * sizeof(vec4), bufMask);

	if (color_array != NULL)
	{
		for (unsigned int i = 0; i < num; i++)
		{
			color_array[i].r = 0.0f;
			color_array[i].g = 0.0f;
			color_array[i].b = 0.5f;
			color_array[i].a = 1.0f;
		}
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	// bind the SSBOs to the labeled "binding" in the compute shader using assigned layout labels
	// this is similar to mapping data to attribute variables in the vertex shader
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, pos_ssbo);    // 4 - layout id for positions in compute shader 
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, color_ssbo);	// 5 - layout id for colors in compute shader 

	// Reset VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, pos_ssbo);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, NULL); // 0 - the layout id in vertex shader
	glBindBuffer(GL_ARRAY_BUFFER, color_ssbo);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, NULL); // 1 - the layout id in fragment shader

	// Attributes are disabled by default in OpenGL 4. 
	// We need to explicitly enable each one.
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}

void ParticleSystem::update(float delta_time)
{
	// invoke the compute shader to update the status of particles 
	glUseProgram(cShaderProg.id);
	cShaderProg.setFloat3V("origin", 1, value_ptr(origin));
	cShaderProg.setFloat3V("sphere.position", 1, value_ptr(spherePos));
	cShaderProg.setFloat("sphere.radius", sphereRadius);
	glDispatchCompute((num+128-1)/128, 1, 1); // one-dimentional GPU threading config, 128 threads per group 
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void ParticleSystem::draw(float particle_size, mat4 view_mat, mat4 proj_mat)
{
	glUseProgram(vfShaderProg.id);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	// set the modelview and projection matrices in the vertex shader
	// no layout labels since they're uniform variables
	vfShaderProg.setMatrix4fv("viewMat", 1, glm::value_ptr(view_mat));
	vfShaderProg.setMatrix4fv("projMat", 1, glm::value_ptr(proj_mat));

	// draw particles as points with VAO
	glBindVertexArray(vao);
	glPointSize(particle_size);
	glDrawArrays(GL_POINTS, 0, num);

	glPopMatrix();
}