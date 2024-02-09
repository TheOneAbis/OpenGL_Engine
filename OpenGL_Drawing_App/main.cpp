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
using namespace std;

// the window's width and height
glm::vec2 windowSize(800, 600);

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;
};

vector<vector<Vertex>> drawVertGroups;
Vertex mousePos;

glm::vec3 drawColor;
GLenum drawMode = GL_POINTS;
float pointSize = 1.f;

void Menu(int value)
{
    switch (value) 
    {
    case 0: // clear
        drawVertGroups.clear();
        glutPostRedisplay();
        break;
    case 1: //exit
        exit(0);

        // Colors
    case 2: // red
        drawColor = glm::vec3(1.f, 0.f, 0.f);
        glutPostRedisplay();
        break;
    case 3: // green
        drawColor = glm::vec3(0.f, 1.f, 0.f);
        glutPostRedisplay();
        break;
    case 4: // blue
        drawColor = glm::vec3(0.f, 0.f, 1.f);
        glutPostRedisplay();
        break;
    case 5: // black
        drawColor = glm::vec3(0.f, 0.f, 0.f);
        glutPostRedisplay();
        break;

        // Draw Modes
    case 6:
        drawMode = GL_POINTS;
        glutPostRedisplay();
        break;
    case 7:
        drawMode = GL_LINE_STRIP;
        glutPostRedisplay();
        break;
    case 8:
        drawMode = GL_TRIANGLES;
        glutPostRedisplay();
        break;
    case 9:
        drawMode = GL_QUADS;
        glutPostRedisplay();
        break;
    case 10:
        drawMode = GL_TRIANGLE_FAN;
        glutPostRedisplay();
        break;

        // Line Widths
    case 11:
        pointSize = 1.f;
        break;
    case 12:
        pointSize = 3.f;
        break;
    case 13:
        pointSize = 5.f;
        break;

    default:
        break;
    }
    mousePos.color = drawColor;
}

void init()
{
    // create right click menu
    int colorMenu = glutCreateMenu(Menu);
    glutAddMenuEntry("Red", 2);
    glutAddMenuEntry("Green", 3);
    glutAddMenuEntry("Blue", 4);
    glutAddMenuEntry("Black", 5);

    int drawMenu = glutCreateMenu(Menu);
    glutAddMenuEntry("Points", 6);
    glutAddMenuEntry("Lines", 7);
    glutAddMenuEntry("Triangles", 8);
    glutAddMenuEntry("Quads", 9);
    glutAddMenuEntry("Polygon", 10);

    int widthMenu = glutCreateMenu(Menu);
    glutAddMenuEntry("Small", 11);
    glutAddMenuEntry("Medium", 12);
    glutAddMenuEntry("Large", 13);

    glutCreateMenu(Menu);
    glutAddMenuEntry("Clear", 0);
    glutAddSubMenu("Colors", colorMenu);
    glutAddSubMenu("Draw Type", drawMenu);
    glutAddSubMenu("Line Width", widthMenu);
    glutAddMenuEntry("Exit", 1);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

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

    glPointSize(pointSize);
    glLineWidth(pointSize);

    // draw verts
    for (auto& drawVerts : drawVertGroups)
    {
        glBegin(drawMode);
        for (int i = 0; i < drawVerts.size(); i++)
        {
            float pos[2] = { drawVerts[i].pos.x, drawVerts[i].pos.y };
            glColor3f(drawVerts[i].color.x, drawVerts[i].color.y, drawVerts[i].color.z);
            glVertex2fv(pos);
        }
        glEnd();
    }

    glEnd();

    if (!drawVertGroups.empty())
    {
        // Handle drawing an incomplete poly
        glBegin(drawMode == GL_POINTS ? GL_POINTS : GL_LINE_STRIP);

        vector<Vertex> lastVerts = drawVertGroups.back();
        auto lastV = lastVerts.end();

        if ((drawMode == GL_TRIANGLES || drawMode == GL_TRIANGLE_FAN) && lastVerts.size() > 2)
        {
            lastV = lastVerts.end() - 3, lastVerts.begin();
        }
        else if (drawMode == GL_QUADS && lastVerts.size() > 3)
        {
            lastV = lastVerts.end() - 4, lastVerts.begin();
        }
        else if (!lastVerts.empty())
        {
            lastV = lastVerts.end() - 1;
        }

        for (lastV; lastV < lastVerts.end(); lastV++)
        {
            Vertex v = *lastV;
            float lastPos[2] = { v.pos.x, v.pos.y };
            glColor3f(v.color.x, v.color.y, v.color.z);
            glVertex2fv(lastPos);
        }

        float pos[2] = { mousePos.pos.x, mousePos.pos.y };
        glColor3f(mousePos.color.x, mousePos.color.y, mousePos.color.z);
        glVertex2fv(pos);

        glEnd();
    }
    
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

void processKeyInput(unsigned char key, int xMouse, int yMouse)
{
    switch (key)
    {
    case 27: // exit the program
        exit(0);
        break;

    default: // if any key was pressed, break the draw here
        if (!drawVertGroups.back().empty())
            drawVertGroups.push_back(vector<Vertex>());
        break;
    }
}

//! button
//!     - 0 = left click
//!     - 2 = right click
//! state
//!     - 0 = down
//!     - 1 = up
void processMouse(int button, int state, int x, int y)
{
    // left click down
    if (!(state || button))
    {
        Vertex newVert;
        newVert.pos = (glm::vec2(x, y) / windowSize) * 2.f - 1.f;
        newVert.pos.y *= -1.f;
        newVert.color = drawColor;

        if (drawVertGroups.empty())
            drawVertGroups.push_back(vector<Vertex>());

        drawVertGroups.back().push_back(newVert);
    }
}

void MouseMotion(int x, int y)
{
    mousePos.pos = (glm::vec2(x, y) / windowSize) * 2.f - 1.f;
    mousePos.pos.y *= -1.f;
    glutPostRedisplay();
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

    glutPassiveMotionFunc(MouseMotion);

    // Processing input
    glutKeyboardFunc(processKeyInput);
    glutMouseFunc(processMouse);

    init();

    //start the glut main loop
    glutMainLoop();

    return 0;
}