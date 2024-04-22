#pragma comment(lib, "ABCore.lib")

#include <GL/glew.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// GLM math library
#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include <GLFW/glfw3.h>

#include <iostream>
#include <ABCore/Scene.h>
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

Transform camTM;
glm::vec2 camEulers;
float dt, oldT;
float camSpeed = 30.f;

void init()
{
    stbi_set_flip_vertically_on_load(true);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    
    oldT = glfwGetTime();

    // set up shader
    shader = Shader("../ABCore/Shaders/vertex.vert", "../ABCore/Shaders/frag_lit_pbr.frag");

    // set up scene models
    Scene& scene = Scene::Get();
    GameObject* firTree = scene.Add(GameObject("../Assets/Fir_Tree.fbx", "Fir Tree"));
    firTree->SetWorldTM({ 10, 0, -20.f }, glm::quat({ 0, 0.7, 0 }), { 0.1, 0.1, 0.1 });
    for (Mesh& m : firTree->GetMeshes())
        m.AddTexture("texture_diffuse", "tree_diffuse.png", "../Assets");

    GameObject* poplarTree = scene.Add(GameObject("../Assets/Poplar_Tree.fbx", "Poplar Tree"));
    poplarTree->SetWorldTM({ 40, 0, -20 }, glm::quat({ 0, 0, 0 }), { 0.1, 0.1, 0.1 });
    for (Mesh& m : poplarTree->GetMeshes())
        m.AddTexture("texture_diffuse", "tree_diffuse.png", "../Assets");

    GameObject* palmTree = scene.Add(GameObject("../Assets/Palm_Tree.fbx", "Palm Tree"));
    palmTree->SetWorldTM({ 50, 0, -45 }, glm::quat({ 0, 0, 0 }), { 0.1, 0.1, 0.1 });
    for (Mesh& m : palmTree->GetMeshes())
        m.AddTexture("texture_diffuse", "tree_diffuse.png", "../Assets");

    GameObject* oakTree = scene.Add(GameObject("../Assets/Oak_Tree.fbx", "Oak Tree"));
    oakTree->SetWorldTM({ 25, 0, -60 }, glm::quat({ 0, 0, 0 }), { 0.1, 0.1, 0.1 });
    for (Mesh& m : oakTree->GetMeshes())
        m.AddTexture("texture_diffuse", "tree_diffuse.png", "../Assets");

    // WALLS
    vector<Mesh> plane = { Mesh(
        {
            { glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 0.f) },
            { glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(20.f, 0.f) },
            { glm::vec3(1.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(20.f, 20.f) },
            { glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 20.f) }
        },
        { 0, 1, 2, 0, 2, 3 },
        vector<Texture>()) };

    vector<int> indices = { 0, 1, 2, 0, 2, 3 };
    GameObject* ground = scene.Add(GameObject("../Assets/Terrain.fbx", "Ground"));
    ground->SetWorldTM(Transform({ 400, -30, -425 }, glm::quat(), { 0.1, 0.1, 0.1 }));
    for (Mesh& m : ground->GetMeshes())
        m.AddTexture("texture_diffuse", "forest_ground.png", "../Assets");

    // set up lights
    Light point = {};
    point.Type = LIGHT_TYPE_POINT;
    point.Color = glm::vec3(1, 1, 1);
    point.Intensity = 1;
    point.Position = glm::vec3(2, 2, -2.5);
    point.Range = 20;
    lights.push_back(point);

    Light sun = {};
    sun.Type = LIGHT_TYPE_DIRECTIONAL;
    sun.Color = glm::vec3(1, 1, 1);
    sun.Intensity = 0.5f;
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
    float speed = camSpeed;
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
    if (Input::Get().KeyDown(GLFW_KEY_LEFT_SHIFT))
        speed *= 2.f;

    glm::vec3 t = glm::normalize(camVel);
    if (!isnan(t.x))
        camTM.Translate(t * camTM.GetRotation() * dt * speed);

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

    Scene::Get().Render(shader);
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