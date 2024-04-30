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
Shader shader, ssrShader;
Mesh ssrTri;
unsigned int framebuffer;
unsigned int colorTex, posTex, normalTex, specTex;
unsigned int depthStencil;

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
    shader = Shader("C:/Users/arjun/source/repos/OpenGL_Engine/ABCore/Shaders/vertex.vert", "C:/Users/arjun/source/repos/OpenGL_Engine/ABCore/Shaders/frag_lit_pbr.frag");
    ssrShader = Shader("C:/Users/arjun/source/repos/OpenGL_Engine/ABCore/Shaders/vert_screen.vert", "C:/Users/arjun/source/repos/OpenGL_Engine/ABCore/Shaders/frag_ssr.frag");

    // set up scene models
    Scene& scene = Scene::Get();
    GameObject* firTree = scene.Add(GameObject("C:/Users/arjun/source/repos/OpenGL_Engine/Assets/Fir_Tree.fbx", "Fir Tree"));
    firTree->SetWorldTM({ 1, 0, -2.f }, glm::quat({ 0, 0.7, 0 }), { 0.01, 0.01, 0.01 });
    for (Mesh& m : firTree->GetMeshes())
        m.AddTexture("texture_diffuse", "tree_diffuse.png", "C:/Users/arjun/source/repos/OpenGL_Engine/Assets/");

    GameObject* poplarTree = scene.Add(GameObject("C:/Users/arjun/source/repos/OpenGL_Engine/Assets/Poplar_Tree.fbx", "Poplar Tree"));
    poplarTree->SetWorldTM({ 4, 0, -2 }, glm::quat({ 0, 0, 0 }), { 0.01, 0.01, 0.01 });
    for (Mesh& m : poplarTree->GetMeshes())
        m.AddTexture("texture_diffuse", "tree_diffuse.png", "C:/Users/arjun/source/repos/OpenGL_Engine/Assets");

    GameObject* palmTree = scene.Add(GameObject("C:/Users/arjun/source/repos/OpenGL_Engine/Assets/Palm_Tree.fbx", "Palm Tree"));
    palmTree->SetWorldTM({ 5, 0, -4.5 }, glm::quat({ 0, 0, 0 }), { 0.01, 0.01, 0.01 });
    for (Mesh& m : palmTree->GetMeshes())
        m.AddTexture("texture_diffuse", "tree_diffuse.png", "C:/Users/arjun/source/repos/OpenGL_Engine/Assets");

    GameObject* oakTree = scene.Add(GameObject("C:/Users/arjun/source/repos/OpenGL_Engine/Assets/Oak_Tree.fbx", "Oak Tree"));
    oakTree->SetWorldTM({ 2.5, 0, -6 }, glm::quat({ 0, 0, 0 }), { 0.01, 0.01, 0.01 });
    for (Mesh& m : oakTree->GetMeshes())
        m.AddTexture("texture_diffuse", "tree_diffuse.png", "C:/Users/arjun/source/repos/OpenGL_Engine/Assets");

    vector<int> indices = { 0, 1, 2, 0, 2, 3 };
    GameObject* ground = scene.Add(GameObject("C:/Users/arjun/source/repos/OpenGL_Engine/Assets/Terrain.fbx", "Ground"));
    ground->SetWorldTM(Transform({ 40, -3, -42.5 }, glm::quat(), { 0.01, 0.01, 0.01 }));
    for (Mesh& m : ground->GetMeshes())
        m.AddTexture("texture_diffuse", "forest_ground.png", "C:/Users/arjun/source/repos/OpenGL_Engine/Assets");

    GameObject* water = Scene::Get().Add(GameObject({ Mesh(
        {
            { glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 0.f) },
            { glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 0.f) },
            { glm::vec3(1.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 1.f) },
            { glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 1.f) }
        },
        { 0, 1, 2, 0, 2, 3 },
        vector<Texture>()) }, "Floor"));
    water->SetWorldTM(Transform({ -7, -1.5f, 8 }, glm::quat(), { 25, 1, 25 }));
    water->GetMaterial().albedo = { 0.0f, 0.0f, 0.5f };
    water->GetMaterial().roughness = 0.f;

    // set up lights
    Light point = {};
    point.Type = LIGHT_TYPE_POINT;
    point.Color = glm::vec3(1, 1, 1);
    point.Intensity = 1;
    point.Position = glm::vec3(2, 2, -2.5);
    point.Range = 10;
    lights.push_back(point);

    Light sun = {};
    sun.Type = LIGHT_TYPE_DIRECTIONAL;
    sun.Color = glm::vec3(1, 1, 1);
    sun.Intensity = 0.5f;
    sun.Direction = { 1, -1, 0.5f };
    lights.push_back(sun);

    ssrTri = Mesh(
        {
            { glm::vec3(-1, 1, 0), glm::vec3(), glm::vec2() },
            { glm::vec3(-1, -3, 0), glm::vec3(), glm::vec2() },
            { glm::vec3(3, 1, 0), glm::vec3(), glm::vec2() }
        },
        { 0, 1, 2 }, {});

    // Create framebuffers and textures to render to
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // generate textures for rendering to
    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // attach new texture to currently bound framebuffer object
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);

    glGenTextures(1, &normalTex);
    glBindTexture(GL_TEXTURE_2D, normalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalTex, 0);

    glGenTextures(1, &posTex);
    glBindTexture(GL_TEXTURE_2D, posTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, posTex, 0);

    glGenTextures(1, &specTex);
    glBindTexture(GL_TEXTURE_2D, specTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, specTex, 0);

    // render buffer instead of texture for depth/stencil; not expecting to need to sample from this, so an RBO is faster
    glGenRenderbuffers(1, &depthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0); // unbind
    // attach new renderbuffer to currently bound framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil);

    // configure framebuffer with these textures
    unsigned int DrawBuffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4, DrawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        cout << "ERROR: Framebuffer shat itself!" << endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    glm::mat4 view = glm::lookAt(camTM.GetTranslation(), camTM.GetTranslation() - camTM.GetForward(), glm::vec3(0.f, 1.f, 0.f));
    glm::mat4 proj = glm::perspective(glm::radians(80.f), (float)width / (float)height, 0.1f, 1000.f);

    // set to render to the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // use normal shader to render colors, normals and positions to textures
    shader.use();

    shader.SetMatrix4x4("projection", proj);
    shader.SetMatrix4x4("view", view);
    shader.SetVector3("cameraPosition", camTM.GetTranslation());
    shader.SetVector3("ambient", glm::vec3(0.05f, 0.05f, 0.05f));

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

    // render to the screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glClearColor(1.f, 1.f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    // use SSR shader to use newly rendered textures from above for the final result
    ssrShader.use();
    glm::vec4 v = proj * glm::vec4(0.5f, 0, -0.5, 1);
    glm::vec3 v3 = glm::vec3(v);
    v3 /= v.w;
    //cout << v3.x << " " << v3.y << " " << v3.z << " " << " " << endl;
    ssrShader.SetMatrix4x4("projection", proj);
    ssrShader.SetFloat("resolution", 0.3f);
    ssrShader.SetInt("steps", 10);
    ssrShader.SetFloat("thickness", 0.3f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, posTex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, specTex);
    
    ssrTri.Draw(ssrShader);
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