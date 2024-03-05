#pragma comment(lib, "ABCore.lib")

#include <GL/glew.h>

// GLM math library
#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#ifdef __APPLE__
#include <GLUT/glut.h> // include glut for Mac
#else
#include <GL/freeglut.h> //include glut for Windows
#endif

#include <iostream>
#include <ABCore/GameObject.h>
#include <ABCore/Input.h>

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
std::vector<GameObject> gameObjects;

Transform camTM;
glm::vec2 camEulers;
float dt, oldT;

void init()
{
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    oldT = glutGet(GLUT_ELAPSED_TIME) / 1000.f;

    // set up shader
    shader = Shader("../ABCore/Shaders/vertex.vert", "../ABCore/Shaders/frag_lit_pbr.frag");

    // set up scene models
    gameObjects.push_back(GameObject("../Assets/cube.obj", "TallCube"));
    gameObjects.back().SetWorldTM({ -0.2, 0, -3.f }, glm::quat({0, 0.7, 0}), { 0.5, 1, 0.5 });
    gameObjects.back().GetMaterial().roughness = 0.4f;

    gameObjects.push_back(GameObject("../Assets/cube.obj", "YellowCube"));
    gameObjects.back().SetWorldTM({ 1.3, -0.5, -3.f }, glm::quat(), { 0.5, 0.5, 0.5 });
    gameObjects.back().GetMaterial().albedo = { 1, 1, 0 };
    gameObjects.back().GetMaterial().roughness = 0.3f;

    // WALLS
    vector<Mesh> plane = { Mesh(
        {
            { glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 0.f) },
            { glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 0.f) },
            { glm::vec3(1.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 1.f) },
            { glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 1.f) }
        },
        { 0, 1, 2, 0, 2, 3 },
        vector<Texture>()) };

    vector<int> indices = { 0, 1, 2, 0, 2, 3 };
    gameObjects.push_back(GameObject(plane, "Floor"));
    gameObjects.back().SetWorldTM(Transform({ -1, -1, -1 }, glm::quat(), { 3, 1, 3 }));

    gameObjects.push_back(GameObject(plane, "Ceiling"));
    gameObjects.back().SetWorldTM(Transform({ 2, 2, -1 }, glm::quat({ 0, 0, glm::pi<float>() }), { 3, 1, 3 }));

    gameObjects.push_back(GameObject(plane, "BackWall"));
    gameObjects.back().SetWorldTM(Transform({ -1, -1, -4 }, glm::quat({ glm::pi<float>() / 2.f, 0, 0 }), {3, 1, 3}));

    gameObjects.push_back(GameObject(plane, "FrontWall"));
    gameObjects.back().SetWorldTM(Transform({ -1, 2, -1 }, glm::quat({ -glm::pi<float>() / 2.f, 0, 0 }), { 3, 1, 3 }));

    gameObjects.push_back(GameObject(plane, "LeftWall"));
    gameObjects.back().SetWorldTM(Transform({ -1, 2, -1 }, glm::quat({ 0, 0, -glm::pi<float>() / 2.f }), {3, 1, 3}));
    gameObjects.back().GetMaterial().albedo = { 0, 0, 1 };

    gameObjects.push_back(GameObject(plane, "RightWall"));
    gameObjects.back().SetWorldTM(Transform({ 2, -1, -1 }, glm::quat({ 0, 0, glm::pi<float>() / 2.f }), { 3, 1, 3 }));
    gameObjects.back().GetMaterial().albedo = { 1, 0, 0 };

    // set up lights
    Light point = {};
    point.Type = LIGHT_TYPE_POINT;
    point.Color = glm::vec3(1, 1, 1);
    point.Intensity = 1.5;
    point.Position = glm::vec3(0.5, 2, -2.5);
    point.Range = 4;
    lights.push_back(point);
}

void Tick()
{
    float newT = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
    dt = newT - oldT;
    oldT = newT;

    if (Input::Get().MouseButtonDown(2))
    {
        camEulers += Input::Get().GetMouseDelta();
        camTM.SetRotationEulersZYX(glm::vec3(camEulers.y * 0.0025f, camEulers.x * 0.0025f, 0.f));
    }

    glm::vec3 camVel = {};
    if (Input::Get().KeyDown('w'))
        camVel.z = -1.f;
    if (Input::Get().KeyDown('a'))
        camVel.x = -1.f;
    if (Input::Get().KeyDown('s'))
        camVel.z = 1.f;
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

    shader.SetMatrix4x4("projection", glm::perspective(glm::radians(80.f), (float)width / (float)height, 0.1f, 1000.f));
    shader.SetMatrix4x4("view", glm::lookAt(camTM.GetTranslation(), camTM.GetTranslation() - camTM.GetForward(), glm::vec3(0.f, 1.f, 0.f)));
    shader.SetVector3("cameraPosition", camTM.GetTranslation());
    shader.SetVector3("ambient", glm::vec3(0.0f, 0.0f, 0.0f));

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

    for (auto& obj : gameObjects)
    {
        obj.Draw(shader);
    }

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

int main(int argc, char* argv[])
{
    //initialize GLUT
    glutInit(&argc, argv);

    // double bufferred
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    //set the initial window size
    glutInitWindowSize((int)width, (int)height);

    // create the window with a title
    glutCreateWindow("Radiosity");

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