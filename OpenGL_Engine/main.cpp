#include <GL/glew.h>

// GLM math library
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#ifdef __APPLE__
#include <GLUT/glut.h> // include glut for Mac
#else
#include <GL/freeglut.h> //include glut for Windows
#endif

#include <iostream>
#include <vector>

#include "../Shader.h"

using namespace std;

// the window's width and height
int width = 800, height = 600;
float dt, oldT;
Shader shader("vertex_2d.vert", "fragment_2d.frag");

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
};

vector<Vertex> drawVerts;

void init()
{
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    oldT = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
}

void Tick()
{
    float newT = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
    dt = newT - oldT;
    oldT = newT;

    glutPostRedisplay();
}

// called when the GL context need to be rendered
void display(void)
{
    // clear the screen to white, which is the background color
    glClearColor(0.1, 0.1, 0.1, 0.0);

    // clear the buffer stored for drawing
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use the shader program
    shader.use();
    

    glutSwapBuffers();
}

// called when window is first created or when window is resized
void reshape(int w, int h)
{
    // update thescreen dimensions
    width = w;
    height = h;

    /* tell OpenGL to use the whole window for drawing */
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    glutPostRedisplay();
}

void processKeyInput(unsigned char key, int xMouse, int yMouse)
{
}

void processKeyUpInput(unsigned char key, int xMouse, int yMouse)
{
}

void processMouse(int button, int state, int x, int y)
{

}

int main(int argc, char* argv[])
{
    //initialize GLUT
    glutInit(&argc, argv);

    // double bufferred
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    //set the initial window size
    glutInitWindowSize((int)width, (int)height);

    // create the window with a title
    glutCreateWindow("OpenGL Program");

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        // error occurred
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    init();

    /* --- register callbacks with GLUT --- */

    //register function to handle window resizes
    glutReshapeFunc(reshape);

    //register function that draws in the window
    glutDisplayFunc(display);

    glutIdleFunc(Tick);

    // Processing input
    glutKeyboardFunc(processKeyInput);
    glutKeyboardUpFunc(processKeyUpInput);
    glutMouseFunc(processMouse);

    //start the glut main loop
    glutMainLoop();

    return 0;
}