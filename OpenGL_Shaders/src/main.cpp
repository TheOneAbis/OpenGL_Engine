// This OpenGL project demonstrates a simple interactive camera implementation and a few vertex and fragment shaders. 
// Two modes of camera controls (press c to switch between two modes): 
// 1. Focus Mode: Holding ALT and Left Mouse Button (LMB) and moving the mouse will rotate the camera about the LookAt Position
//                Holding ALT and MMB and moving the mouse will pan the camera.
//                Holding ALT and RMB and moving the mouse will zoom the camera.
// 2. First-Person Mode: Pressing W, A, S, or D will move the camera
//                       Holding LMB and moving the mouse will roate the camera.
// Basic shader - demonstrate the basic use of vertex and fragment shader
// Displacement shader - a special fireball visual effects with Perlin noise function
// Toon shading shader - catoonish rendering effects
// Per-vertex shading v.s. per-fragment shading = visual comparison between two types of shading 

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
#include "Mesh.h"
#include "Light.h"

#include <iostream>
using namespace std;
using namespace glm;

int g_winWidth  = 1024;
int g_winHeight = 512;

Camera g_cam;

unsigned char g_keyStates[256];

char v_shader_file[] =
//".\\shaders\\basic.vert";
//".\\shaders\\displacement.vert"; // vertex displacement shader with perlin noise
//".\\shaders\\perVert_lambert.vert"; // basic lambert lighting  
 ".\\shaders\\perFrag_lambert.vert"; // basic lambert lighting with per-fragment implementation
// ".\\shaders\\toon_shading.vert"; // basic toon shading with per-fragment implementation

char f_shader_file[] =
//".\\shaders\\basic.frag";
// ".\\shaders\\displacement.frag"; // vertex displacement shader with perlin noise
// ".\\shaders\\perVert_lambert.frag"; // basic lambert shading 
 ".\\shaders\\perFrag_lambert.frag"; // basic lambert shading with per-fragment implementation
// ".\\shaders\\toon_shading.frag"; // basic toon shading with per-fragment implementation

const char meshFile[128] = 
//"Mesh/sphere.obj";
//"Mesh/bunny2K.obj";
"Mesh/teapot.obj";
//"Mesh/teddy.obj";

Mesh g_mesh[2] {};
vector<Light> pointLights;
bool index = false;

float g_time = 0.0f;

void initialization() 
{    
    g_cam.set(3.0f, 4.0f, 14.0f, 0.0f, 1.0f, -0.5f, g_winWidth, g_winHeight);

	// Create teapots
	g_mesh[0].create(meshFile, v_shader_file, f_shader_file, { 0, 2, 0 }, { 0.5f, 0.5f, 0.5f });
	g_mesh[1].create(meshFile, ".\\shaders\\perVert_lambert.vert", ".\\shaders\\perVert_lambert.frag", { 3, 2, 0 }, { 0.5f, 0.5f, 0.5f });
	
	// Create point lights
	Light point1;
	point1.position = { 3, 0, 3 };
	point1.ambientColor = { 0, 0.15f, 0 };
	point1.diffuseColor = { 1, 1, 0 };
	point1.specularColor = { 1, 0, 0 };
	point1.range = 10.f;
	point1.specularExp = 10.f;
	pointLights.push_back(point1);

	Light point2;
	point2.position = { 1, 0, -2 };
	point2.ambientColor = { 0, 0, 0.15f };
	point2.diffuseColor = { 1, 0, 1 };
	point2.specularColor = { 1, 0, 0 };
	point2.range = 10.f;
	point2.specularExp = 10.f;
	pointLights.push_back(point2);
}

/****** GL callbacks ******/
void initialGL()
{    
	glDisable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	
	glClearColor (1.0f, 1.0f, 1.0f, 0.0f);
	
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
}

void idle()
{
    // add any stuff to update at runtime ....

    g_cam.keyOperation(g_keyStates, g_winWidth, g_winHeight);

	glutPostRedisplay();
}

void display()
{	 
	glClearColor(1.0, 1.0, 1.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
	// add any stuff you'd like to draw	

	glUseProgram(0);
	glDisable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);

	g_cam.drawGrid();
    g_cam.drawCoordinateOnScreen(g_winWidth, g_winHeight);
    g_cam.drawCoordinate();

	glMatrixMode(GL_MODELVIEW);

	glLoadMatrixf(value_ptr(g_cam.viewMat));

	glPushMatrix();
	glTranslatef(pointLights[index].position.x, pointLights[index].position.y, pointLights[index].position.z);
	glutSolidSphere(0.3f, 16, 16);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(pointLights[!index].position.x, pointLights[!index].position.y, pointLights[!index].position.z);
	glutWireSphere(0.3f, 8, 8);
	glPopMatrix();

	g_time = (float)glutGet(GLUT_ELAPSED_TIME)/1000.0f;

	for (auto& m : g_mesh)
	{
		m.draw(vec3(g_cam.eye), g_cam.viewMat, g_cam.projMat, pointLights, g_time);
	}

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
	switch(key) { 
		case 27:
			exit(0);
			break;
		case '1':
			index = 0;
			break;
		case '2':
			index = 1;
			break;
		case 'w':
			pointLights[index].position.z -= 0.1f;
			break;
		case 's':
			pointLights[index].position.z += 0.1f;
			break;
		case 'a':
			pointLights[index].position.x -= 0.1f;
			break;
		case 'd':
			pointLights[index].position.x += 0.1f;
			break;
		case 'q':
			pointLights[index].position.y -= 0.1f;
			break;
		case 'e':
			pointLights[index].position.y += 0.1f;
			break;
	}
}

int main(int argc, char **argv) 
{
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(g_winWidth, g_winHeight);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("VertFrag Shader Example");
	
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