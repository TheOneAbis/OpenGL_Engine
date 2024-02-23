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
#include "../GameObject.h"
#include "../Input.h"

using namespace std;
using namespace AB;

// the window's width and height
int width = 1280, height = 720;

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
GameObject screenTri;
GameObject smallSphere, bigSphere, cone;
GameObject mFloor;
std::vector<GameObject> gameObjects;

Transform camTM;

float dt, oldT;

unsigned int vertices;

void init()
{
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    oldT = glutGet(GLUT_ELAPSED_TIME) / 1000.f;

    // set up shader
    shader = Shader("../Shaders/vert_screen.vert", "../Shaders/frag_raytracing.frag");
    Mesh tri(
        {
            {glm::vec3(-1, 1, 0), glm::vec3(), glm::vec2()},
            {glm::vec3(-1, -3, 0), glm::vec3(), glm::vec2()},
            {glm::vec3(3, 1, 0), glm::vec3(), glm::vec2()}
        }, 
        {0, 1, 2}, {});
    screenTri = GameObject({ tri });
    // set the background color
    screenTri.GetMaterial().albedo = glm::vec3(0.25f, 0.61f, 1.f);

    // set up scene models
    smallSphere = GameObject("Assets/sphere.fbx");
    smallSphere.SetWorldTM({1.5f, 0.f, 3.25f}, glm::quat(), {0.75f, 0.75f, 0.75f});
    smallSphere.GetMaterial().albedo = { 0.3f, 0.3f, 0.1f };

    bigSphere = GameObject("Assets/sphere.fbx");
    bigSphere.SetWorldTM({ 0.f, 0.5f, 2.f }, glm::quat(), {1.f, 1.f, 1.f});
    auto* mat = &bigSphere.GetMaterial();
    mat->albedo = { 0.5f, 0.1f, 0.5f };
    mat->metallic = 0.5f;
   
    cone = GameObject("Assets/cone.obj");
    cone.SetWorldTM({ -1.5f, 0.5f, 3.f }, glm::quat(), {1.f, 1.f, 1.f});
    mat = &cone.GetMaterial();
    mat->albedo = { 0.8f, 0.3f, 0.1f };
    mat->specular = 0.2f;
    
    mFloor = GameObject({ Mesh(
        {
            { glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 0.f) },
            { glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 0.f) },
            { glm::vec3(1.f, 0.f, 1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 1.f) },
            { glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 1.f) }
        }, 
        { 0, 2, 1, 0, 3, 2 }, 
        vector<Texture>()) });
    mFloor.SetWorldTM(Transform({-1, -1, 0}, glm::quat(), {3, 1, 5}));
    mFloor.GetMaterial().albedo = { 0.2f, 0.3f, 0.9f };

    gameObjects = { smallSphere, bigSphere, cone, mFloor };

    // set up lights
    Light point = {};
    point.Type = LIGHT_TYPE_POINT;
    point.Color = glm::vec3(1.f, 1.f, 1.f);
    point.Intensity = 1.f;
    point.Position = glm::vec3(-2, 0, 0);
    point.Range = 10.f;
    lights.push_back(point);

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

    glm::vec3 camVel = {};
    if (Input::Get().KeyDown('w'))
        camVel.z = 1.f;
    if (Input::Get().KeyDown('a'))
        camVel.x = -1.f;
    if (Input::Get().KeyDown('s'))
        camVel.z = -1.f;
    if (Input::Get().KeyDown('d'))
        camVel.x = 1.f;
    if (Input::Get().KeyDown('q'))
        camVel.y = -1.f;
    if (Input::Get().KeyDown('e'))
        camVel.y = 1.f;

    glm::vec3 t = glm::normalize(camVel);
    if (!isnan(t.x))
        camTM.Translate(t * camTM.GetRotation() * dt * 3.f);

    Input::Get().Update();

    glutPostRedisplay();
}

// called when the GL context need to be rendered
void display(void)
{
    // clear the screen to white, which is the background color
    glClearColor(0.1f, 0.1f, 0.1f, 0.f);

    // clear the buffer stored for drawing
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // use the shader program
    shader.use();

    shader.SetMatrix4x4("view", glm::lookAt(camTM.GetTranslation(), camTM.GetTranslation() + camTM.GetForward(), glm::vec3(0.f, 1.f, 0.f)));
    shader.SetMatrix4x4("projection", glm::perspective(glm::radians(80.f), (float)width / (float)height, 0.1f, 1000.f));
    
    
    screenTri.Draw(shader);

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

void processMouse(int button, int state, int x, int y)
{
}

void MouseMotion(int x, int y)
{
    camTM.SetRotationEulersZYX(glm::vec3(-y * 0.0025f, x * 0.0025f, 0.f));
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
    Input::Get().Init();

    //start the glut main loop
    glutMainLoop();

    return 0;
}