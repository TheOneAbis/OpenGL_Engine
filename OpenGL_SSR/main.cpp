#pragma comment(lib, "ABCore.lib")

#include <GL/glew.h>

// GLM math library
#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include <GLFW/glfw3.h>

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
    
    oldT = glfwGetTime();

    // set up shader
    shader = Shader("../ABCore/Shaders/vertex.vert", "../ABCore/Shaders/frag_lit_pbr.frag");

    // set up scene models
    gameObjects.push_back(GameObject("../Assets/Fir_Tree.fbx", "Fir Tree"));
    gameObjects.back().SetWorldTM({ -0.2, 0, -3.f }, glm::quat({ 0, 0.7, 0 }), { 0.1, 0.1, 0.1 });

    gameObjects.push_back(GameObject("../Assets/Poplar_Tree.fbx", "Poplar Tree"));
    gameObjects.back().SetWorldTM({ 50, 0, -20 }, glm::quat({ 0, 0, 0 }), { 0.1, 0.1, 0.1 });

    gameObjects.push_back(GameObject("../Assets/Palm_Tree.fbx", "Palm Tree"));
    gameObjects.back().SetWorldTM({ 75, 0, -50 }, glm::quat({ 0, 0, 0 }), { 0.1, 0.1, 0.1 });

    gameObjects.push_back(GameObject("../Assets/Oak_Tree.fbx", "Oak Tree"));
    gameObjects.back().SetWorldTM({ 15, 0, -90 }, glm::quat({ 0, 0, 0 }), { 0.1, 0.1, 0.1 });

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
    gameObjects.back().SetWorldTM(Transform({ -500, 0, 500 }, glm::quat(), { 1000, 1, 1000 }));

    // set up lights
    Light point = {};
    point.Type = LIGHT_TYPE_POINT;
    point.Color = glm::vec3(1, 1, 1);
    point.Intensity = 1;
    point.Position = glm::vec3(2, 2, -2.5);
    point.Range = 100;
    lights.push_back(point);

    Light sun = {};
    sun.Type = LIGHT_TYPE_DIRECTIONAL;
    sun.Color = glm::vec3(1, 1, 1);
    sun.Intensity = 1.f;
    sun.Direction = {1, -1, 0.5f};
    lights.push_back(sun);
}

void Tick()
{
    float newT = glfwGetTime();
    dt = newT - oldT;
    oldT = newT;

    if (Input::Get().MouseButtonDown(GLFW_MOUSE_BUTTON_2))
    {
        camEulers += Input::Get().GetMouseDelta();
        camTM.SetRotationEulersZYX(glm::vec3(camEulers.y * 0.0025f, camEulers.x * 0.0025f, 0.f));
    }

    glm::vec3 camVel = {};
    if (Input::Get().KeyDown(GLFW_KEY_W))
        camVel.z = -1.f;
    if (Input::Get().KeyDown(GLFW_KEY_A))
        camVel.x = -1.f;
    if (Input::Get().KeyDown(GLFW_KEY_S))
        camVel.z = 1.f;
    if (Input::Get().KeyDown(GLFW_KEY_D))
        camVel.x = 1.f;
    if (Input::Get().KeyDown(GLFW_KEY_Q))
        camVel.y = -1.f;
    if (Input::Get().KeyDown(GLFW_KEY_E))
        camVel.y = 1.f;

    glm::vec3 t = glm::normalize(camVel);
    if (!isnan(t.x))
        camTM.Translate(t * camTM.GetRotation() * dt * 30.f);

    Input::Get().Update();
}

// called when the GL context need to be rendered
void display()
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
}

// called when window is first created or when window is resized
void reshape(GLFWwindow* window, int w, int h)
{
    // update thescreen dimensions
    width = w;
    height = h;

    /* tell OpenGL to use the whole window for drawing */
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

int main(int argc, char* argv[])
{
    // initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // initialize GLEW
    glewExperimental = GL_TRUE;
    GLFWwindow* window = glfwCreateWindow(width, height, "Screen Space Reflections", NULL, NULL);
    if (window == NULL)
    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, reshape);

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        // error occurred
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    // initialize everything else
    init();
    Input::Get().Init(window);

    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
        Tick();
        display();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}