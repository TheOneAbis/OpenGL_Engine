#include <GL/glew.h>

// GLM math library
#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef __APPLE__
#include <GLUT/glut.h> // include glut for Mac
#else
#include <GL/freeglut.h> //include glut for Windows
#endif

#include <iostream>
#include "Model.h"

using namespace std;

// the window's width and height
int width = 800, height = 600;

float floorVerts[] =
{
    0.5f, 0.0f, -5.0f,  0.f, 1.f, 0.f, // far right
    0.5f, 0.0f, 5.0f,   0.f, 1.f, 0.f, // close right
    -0.5f, 0.0f, 5.0f,   0.f, 1.f, 0.f, // close left 
    -0.5f, 0.0f, -5.0f,  0.f, 1.f, 0.f  // far left
};
int floorIndices[] =
{
    0, 3, 1,
    1, 3, 2
};

struct Light
{
    unsigned int Type;         
    glm::vec3 Direction;    
    float Range;
    glm::vec3 Position;
    float Intensity;   
    glm::vec3 Color;
    float SpotFalloff; 
};

enum LightType : int
{
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_POINT,
    LIGHT_TYPE_SPOT
};

vector<Light> lights;
Shader shader;
Model bigSphere, smallSphere;
Mesh mFloor;

glm::vec3 camPos, camVel, camForward;
float dt, oldT;

void init()
{
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    camPos = {};
    camVel = {};
    camForward = glm::vec3(0.f, 0.f, -1.f);
    oldT = glutGet(GLUT_ELAPSED_TIME) / 1000.f;

    shader = Shader("vertex.vert", "fragment.frag");

    bigSphere = Model("sphere.fbx");
    smallSphere = Model("sphere.fbx");

    vector<Vertex> floorVerts;
    floorVerts.push_back({ glm::vec3(2.f, -1.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 0.f) });
    floorVerts.push_back({ glm::vec3(-2.f, -1.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 0.f) });
    floorVerts.push_back({ glm::vec3(-2.f, -1.f, -8.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 1.f) });
    floorVerts.push_back({ glm::vec3(2.f, -1.f, -8.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 1.f) });

    vector<unsigned int> floorIndices;
    floorIndices.push_back(0);
    floorIndices.push_back(2);
    floorIndices.push_back(1);
    floorIndices.push_back(0);
    floorIndices.push_back(3);
    floorIndices.push_back(2);
    mFloor = Mesh(floorVerts, floorIndices, vector<Texture>());

    Light point = {};
    point.Type = LIGHT_TYPE_POINT;
    point.Color = glm::vec3(1.f, 1.f, 1.f);
    point.Intensity = 1.f;
    point.Position = glm::vec3(-2, 0, 0);
    point.Range = 10.f;
    //lights.push_back(point);

    Light dirLight = {};
    dirLight.Type = LIGHT_TYPE_DIRECTIONAL;
    dirLight.Direction = glm::normalize(glm::vec3(-0.3f, -1.f, -0.5f));
    dirLight.Color = glm::vec3(1.f, 1.f, 1.f);
    dirLight.Intensity = 3.f;
    lights.push_back(dirLight);
}

void Tick()
{
    float newT = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
    dt = newT - oldT;
    oldT = newT;

    glm::vec3 t = glm::normalize(camVel);
    if (!isnan(t.x))
        camPos += t * dt * 3.f;

    glutPostRedisplay();
}

// called when the GL context need to be rendered
void display(void)
{
    // clear the screen to white, which is the background color
    glClearColor(0.1, 0.1, 0.1, 0.0);

    // clear the buffer stored for drawing
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glm::mat4x4 world = glm::mat4x4(1.f);
    glm::mat4x4 view = glm::lookAt(camPos, camPos + glm::normalize(camForward), glm::vec3(0.f, 1.f, 0.f));

    // use the shader program
    shader.use();

    // Set vertex uniforms
    shader.SetMatrix4x4("view", view);
    shader.SetMatrix4x4("projection", glm::perspective(glm::radians(80.f), (float)width / (float)height, 0.1f, 1000.f));

    // Set fragment uniforms
    shader.SetFloat("roughness", 0.1f);
    shader.SetFloat("metallic", 0.0f);
    shader.SetVector3("ambient", glm::vec3(0.05f, 0.05f, 0.05f));
    shader.SetVector3("cameraPosition", camPos);

    // Lighting uniform data
    for (unsigned int i = 0; i < lights.size(); i++)
    {
        shader.SetUint("lights[" + to_string(i) + "].Type", lights[i].Type);
        shader.SetVector3("lights[" + to_string(i) + "].Direction", lights[i].Direction);
        shader.SetFloat("lights[" + to_string(i) + "].Range", lights[i].Range);
        shader.SetVector3("lights[" + to_string(i) + "].Position", lights[i].Position);
        shader.SetFloat("lights[" + to_string(i) + "].Intensity", lights[i].Intensity);
        shader.SetVector3("lights[" + to_string(i) + "].Color", lights[i].Color);
        shader.SetFloat("lights[" + to_string(i) + "].SpotFalloff", lights[i].SpotFalloff);
    }

    world = glm::mat4x4();
    shader.SetMatrix4x4("world", world);
    shader.SetMatrix4x4("worldInvTranspose", glm::inverse(glm::transpose(world)));
    shader.SetVector3("albedoColor", glm::vec3(0.f, 0.3f, 0.75f));
    mFloor.Draw(shader);

    world = glm::translate(glm::scale(glm::mat4x4(), glm::vec3(0.5f, 0.5f, 0.5f)), glm::vec3(0.f, 0.f, -2.f));
    shader.SetMatrix4x4("world", world);
    shader.SetMatrix4x4("worldInvTranspose", glm::inverse(glm::transpose(world)));
    shader.SetVector3("albedoColor", glm::vec3(0.3f, 0.3f, 0.1f));
    smallSphere.Draw(shader);

    world = glm::translate(glm::mat4(), glm::vec3(-1.f, -0.5f, -3.f));
    shader.SetMatrix4x4("world", world);
    shader.SetMatrix4x4("worldInvTranspose", glm::inverse(glm::transpose(world)));
    shader.SetFloat("metallic", 0.5f);
    shader.SetVector3("albedoColor", glm::vec3(0.5f, 0.1f, 0.5f));
    bigSphere.Draw(shader);

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
    if (key == 'w')
        camVel.z = -1.f;
    if (key == 'a')
        camVel.x = -1.f;
    if (key == 's')
        camVel.z = 1.f;
    if (key == 'd')
        camVel.x = 1.f;
    if (key == 'e')
        camVel.y = 1.f;
    if (key == 'q')
        camVel.y = -1.f;
}

void processKeyUpInput(unsigned char key, int xMouse, int yMouse)
{
    if (key == 'w' || key == 's')
        camVel.z = 0;
    if (key == 'a' || key == 'd')
        camVel.x = 0;
    if (key == 'q' || key == 'e')
        camVel.y = 0;
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