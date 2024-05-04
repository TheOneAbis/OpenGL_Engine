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

struct Patch
{
    Vertex verts[6];
    glm::vec3 normal;
    glm::vec3 center;
    glm::mat4 views[5];
    glm::vec3 emmision, incident, excident, reflectance;
    glm::vec3 finalCol;
    vector<Patch*> adjacents;
    char id[3];
    unordered_map<Patch*, float> formFactors;
};

// the window's width and height
int width = 1280, height = 720;

Shader shader;

Transform camTM;
glm::vec2 camEulers;
float dt, oldT;
float camSpeed = 3.f;

void init()
{
    stbi_set_flip_vertically_on_load(true);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    oldT = glfwGetTime();

    // set up shader
    shader = Shader("../ABCore/Shaders/vertex.vert", "../ABCore/Shaders/frag_lit_pbr.frag");

    // set up scene model
    GameObject* scene = Scene::Get().Add(GameObject("../Assets/cornell-box-holes2-subdivided2.obj", "Cornell Box"));
    scene->SetWorldTM({3, -2.5f, -2}, glm::quat({ glm::pi<float>() / 2.f, glm::pi<float>(), glm::pi<float>() }));

    // light plane
    vector<Mesh> plane = { Mesh(
        {
            { glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 0.f) },
            { glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 0.f) },
            { glm::vec3(1.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 1.f) },
            { glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 1.f) }
        },
        { 0, 1, 2, 0, 2, 3 },
        vector<Texture>()) };
    GameObject* lightPlane = Scene::Get().Add(GameObject(plane, "Light Plane"));
    lightPlane->SetWorldTM(Transform({ 0.9, 2.97, -4.1 }, glm::quat({ 0, 0, glm::pi<float>() }), glm::vec3(1.4)));
    lightPlane->GetMaterial().emissive = 1;


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
    if (Input::Get().KeyDown(GLFW_KEY_LEFT_SHIFT))
        speed *= 2.f;

    glm::vec3 t = glm::normalize(camVel);
    if (!isnan(t.x))
        camTM.Translate(t * camTM.GetRotation() * dt * speed);

    if (Input::Get().KeyDown(GLFW_KEY_Q))
        camVel.y = -1.f;
    if (Input::Get().KeyDown(GLFW_KEY_E))
        camVel.y = 1.f;
    camTM.Translate({ 0, camVel.y * dt * speed, 0 });

    Input::Get().Update();
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