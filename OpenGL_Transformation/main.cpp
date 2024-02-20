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
#include "../GameObject.h"

using namespace std;
using namespace AB;

// the window's width and height
glm::vec2 windowSize(1280, 720);

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;
};

GameObject body;
Vertex mousePos;

glm::vec3 drawColor;
GLenum drawMode = GL_POINTS;
float pointSize = 1.f;

Shader shader;

void init()
{
    shader = Shader("../Shaders/vertex_basic.vert", "../Shaders/fragment_basic.frag");

    drawColor = glm::vec3();
    mousePos =
    {
        glm::vec2(),
        drawColor
    };
}

// called when the GL context need to be rendered
void display(void)
{
    // clear the screen to white, which is the background color
    glClearColor(1.f, 1.f, 1.f, 0.0);

    // clear the buffer stored for drawing
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    glutSwapBuffers();
}

// called when window is first created or when window is resized
void reshape(int w, int h)
{
    // update thescreen dimensions
    windowSize = glm::vec2(w, h);

    /* tell OpenGL to use the whole window for drawing */
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);

    glutPostRedisplay();
}

//! up - select parent
//! down - select child
//! A/D - cycle children
void processKeyInput(unsigned char key, int xMouse, int yMouse)
{
    switch (key)
    {
    case 27: // exit the program
        exit(0);
        break;
    }
}

int main(int argc, char* argv[])
{
    //initialize GLUT
    glutInit(&argc, argv);

    // double bufferred
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    //set the initial window size
    glutInitWindowSize((int)windowSize.x, (int)windowSize.y);

    // create the window with a title
    glutCreateWindow("Simple OpenGL Drawing Program");

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        // error occurred
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    /* --- register callbacks with GLUT --- */

    glutReshapeFunc(reshape);

    glutDisplayFunc(display);

    // Processing input
    glutKeyboardFunc(processKeyInput);

    init();

    //start the glut main loop
    glutMainLoop();

    return 0;
}