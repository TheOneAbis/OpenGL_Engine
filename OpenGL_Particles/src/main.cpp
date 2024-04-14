#include <GL/glew.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/freeglut.h>
#endif
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/constants.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Camera.h"
#include "Text.h"
#include "ParticleSystem.h"

#include <iostream>
using namespace std;
using namespace glm;

struct Sphere
{
	vec3 position;
	float radius;
};

int g_winWidth = 1024;
int g_winHeight = 512;

Camera g_cam;
Text g_text;

unsigned char g_keyStates[256];

unsigned int curTime = 0; //the milliseconds since the start
unsigned int preTime = 0;
ParticleSystem parSys;

char v_shader_file[] = ".\\shaders\\v_shader.vert";
char f_shader_file[] = ".\\shaders\\f_shader.frag";
char c_shader_file[] = ".\\shaders\\c_shader.comp";

uvec2 resolution = { 64, 32 };
vec3 origin = vec3(0, 5, 20);
vec3 minPt = vec3(-10, 0, -5);
vec3 maxPt = vec3(10, 10, -5);

void initialization()
{
	Sphere s;
	s.position = vec3(0, 5, -8);
	s.radius = 2;
	parSys.create(resolution, minPt, maxPt, origin, s.position, s.radius,
		c_shader_file, v_shader_file, f_shader_file);

	g_cam.set(38.0f, 13.0f, 4.0f, 0.0f, 0.0f, 0.0f, g_winWidth, g_winHeight, 45.0f, 0.01f, 10000.0f);
	g_text.setColor(0.0f, 0.0f, 0.0f);

	// add any stuff you want to initialize ...
}

/****** GL callbacks ******/
void initialGL()
{
	glDisable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);

	glClearColor(1.0f, 1.0f, 1.0f, 0.0f);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void idle()
{
	// add any stuff to update at runtime ....
	curTime = glutGet(GLUT_ELAPSED_TIME);
	float deltaT = (float)(curTime - preTime) / 1000.0f; // in seconds
	parSys.update(deltaT);

	g_cam.keyOperation(g_keyStates, g_winWidth, g_winHeight);

	glutPostRedisplay();

	preTime = curTime;
}

void display()
{
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(0);
	glDisable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	parSys.draw(1.0f, g_cam.viewMat, g_cam.projMat);

	g_cam.drawGrid();
	g_cam.drawCoordinateOnScreen(g_winWidth, g_winHeight);
	g_cam.drawCoordinate();

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(value_ptr(g_cam.viewMat));

	// draw spheres and lines
	glPushMatrix();
	glTranslatef(parSys.origin.x, parSys.origin.y, parSys.origin.z);
	glColor3f(1, 0, 0);
	glutSolidSphere(0.3f, 16, 16);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(parSys.spherePos.x, parSys.spherePos.y, parSys.spherePos.z);
	glColor3f(0, 1, 0);
	glutWireSphere(parSys.sphereRadius, 16, 16);
	glPopMatrix();

	glColor3f(1, 0, 0);
	glBegin(GL_LINES);
	glVertex3f(origin.x, origin.y, origin.z);
	glVertex3f(minPt.x, minPt.y, minPt.z);

	glVertex3f(origin.x, origin.y, origin.z);
	glVertex3f(minPt.x, maxPt.y, minPt.z);

	glVertex3f(origin.x, origin.y, origin.z);
	glVertex3f(maxPt.x, maxPt.y, minPt.z);

	glVertex3f(origin.x, origin.y, origin.z);
	glVertex3f(maxPt.x, minPt.y, minPt.z);
	glEnd();


	// display the text
	if (g_cam.isFocusMode()) {
		string str = "Cam mode: Focus";
		g_text.draw(10, 30, const_cast<char*>(str.c_str()), g_winWidth, g_winHeight);
	}
	else if (g_cam.isFPMode()) {
		string str = "Cam mode: FP";
		g_text.draw(10, 30, const_cast<char*>(str.c_str()), g_winWidth, g_winHeight);
	}
	g_text.draw(10, 50, const_cast<char*>(string("Resolution: " + std::to_string(parSys.resolution.x) + 'x' + std::to_string(parSys.resolution.y)).c_str()), g_winWidth, g_winHeight);
	g_text.draw(10, 70, const_cast<char*>(string("# of Particles: " + std::to_string(parSys.num)).c_str()), g_winWidth, g_winHeight);

	glPopMatrix();

	glutSwapBuffers();
}

void reshape(int w, int h)
{
	g_winWidth = w;
	g_winHeight = h;
	if (h == 0) {
		h = 1;
	}
	g_cam.setProjectionMatrix(g_winWidth, g_winHeight);
	g_cam.setViewMatrix(); 
	glViewport(0, 0, w, h);
}

void mouse(int button, int state, int x, int y)
{
	g_cam.mouseClick(button, state, x, y, g_winWidth, g_winHeight);
}

void motion(int x, int y)
{
	g_cam.mouseMotion(x, y, g_winWidth, g_winHeight);
}

void keyup(unsigned char key, int x, int y)
{
	g_keyStates[key] = false;
}

void keyboard(unsigned char key, int x, int y)
{
	g_keyStates[key] = true;
	switch (key) {
	case 27:
		exit(0);
		break;
	case 'c': // switch cam control mode
		g_cam.switchCamMode();
		glutPostRedisplay();
		break;
	case ' ':
		g_cam.PrintProperty();
		break;
	case 'w':
		parSys.spherePos.z += 0.1f;
		break;
	case 'a':
		parSys.spherePos.x -= 0.1f;
		break;
	case 's':
		parSys.spherePos.z -= 0.1f;
		break;
	case 'd':
		parSys.spherePos.x += 0.1f;
		break;
	case 'q':
		parSys.spherePos.y -= 0.1f;
		break;
	case 'e':
		parSys.spherePos.y += 0.1f;
		break;
	case '+':
		if (resolution.x < 8192)
		{
			resolution *= 2.f;
			parSys.ResetBuffers(resolution);
		}
		break;
	case '-':
		if (resolution.x > 16)
		{
			resolution /= 2.f;
			parSys.ResetBuffers(resolution);
		}
		break;
	}
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(g_winWidth, g_winHeight);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("Compute Shader Example: A Simple particle System");

	glewInit();
	initialGL();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutKeyboardUpFunc(keyup);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);

	initialization();

	glutMainLoop();
	return EXIT_SUCCESS;
}