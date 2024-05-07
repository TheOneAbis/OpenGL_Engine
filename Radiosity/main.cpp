// --------------------------------------------------------------------------------------------------- \\
//                                                                                                     \\
//                            ================ RADIOSITY =================                             \\
//                                                                                                     \\
// --------------------------------------------------------------------------------------------------- \\
//                                                                                                     \\
// My implementation of radiosity works as follows:                                                    \\
//                                                                                                     \\
// 1. Before the first iteration, add all patches' emissions to their final color.                     \\
// 2. At the beginning of an iteration, all patches emit their "emission" light to all other patches.  \\
// 3. At the end of an iteration, the emission of all patches is set to (0,0,0).                       \\
// 4. Resulting exitance from that iteration becomes the patch's new emission for the next iteration.  \\
// 5. This exitance is also added to that patch's final color.                                         \\
// 6. The exitences are all set to (0,0,0) for the next iteration.                                     \\
// 7. Repeat from step 2 for as many iterations as you want.                                           \\
//                                                                                                     \\
// --------------------------------------------------------------------------------------------------- \\

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
#include <sstream>

#include <ABCore/Scene.h>
#include <ABCore/Input.h>

using namespace std;
using namespace AB;

struct Patch
{
    Mesh mesh;
    glm::vec3 normal;
    glm::vec3 center;
    glm::mat4 views[5];
    
    // light emitted from this patch. 
    glm::vec3 emission; 
    // incoming light from other patches i, which is sum of emission(i) * ffs(ij) * (A(i)/A(j))
    glm::vec3 incident;
    // essentially the patch's base color.
    glm::vec3 reflectance = glm::vec3(1);
    // the final color to display at the end of all iterations
    glm::vec3 finalColor;

    vector<Patch*> adjacents;
    float area;
    unsigned int id;

    // maps each other patch in the scene to their form factors
    unordered_map<unsigned int, float> formFactors;
};

// the window's width and height
int width = 1280, height = 720;

Shader shader;
Shader compute;
vector<Patch> patches;
GLFWwindow* window;

Transform camTM;
glm::vec2 camEulers;
float dt, oldT;
float camSpeed = 3.f;

Patch CreatePatch(glm::vec3 positions[4], glm::vec3 emission)
{
    Patch p = {};
    glm::vec3 v0 = positions[1] - positions[0];
    glm::vec3 v1 = positions[2] - positions[1];
    glm::vec3 v2 = positions[3] - positions[2];
    glm::vec3 v3 = positions[0] - positions[3];

    // normal
    p.normal = glm::normalize(glm::cross(v0, v1));

    // center
    for (int i = 0; i < 4; i++)
        p.center += positions[i];
    p.center /= 4.f;

    // mesh
    p.mesh = Mesh(
        vector<Vertex>({ 
            { positions[0], p.normal, { 0, 1 } },
            { positions[1], p.normal, { 1, 1 } }, 
            { positions[2], p.normal, { 1, 0 } }, 
            { positions[3], p.normal, { 0, 0 } }
        }), 
        vector<unsigned int>({ 0, 1, 2, 0, 2, 3 }), 
        vector<Texture>());

    // views
    p.views[0] = glm::lookAt(p.center, p.center + p.normal, glm::vec3(p.normal.y, -p.normal.x, p.normal.z));
    p.views[1] = glm::lookAt(p.center, p.center + glm::vec3(p.normal.y, -p.normal.x, p.normal.z), p.normal);
    p.views[2] = glm::lookAt(p.center, p.center + glm::vec3(-p.normal.y, p.normal.x, p.normal.z), p.normal);
    p.views[3] = glm::lookAt(p.center, p.center + glm::vec3(p.normal.x, p.normal.z, -p.normal.y), p.normal);
    p.views[4] = glm::lookAt(p.center, p.center + glm::vec3(p.normal.x, -p.normal.z, p.normal.y), p.normal);

    // cross(tri1) / 2 + cross(tri2) / 2
    p.area = glm::length(glm::cross(v0, v1)) / 2.f + glm::length(glm::cross(v2, v3)) / 2.f;
    p.emission = emission;
    p.id = patches.size();

    return p;
}

glm::vec3 ToColor(unsigned int id)
{
    return glm::vec3(id / (255 * 255) % 255, id / 255 % 255, id % 255);
}
unsigned int FromColor(glm::vec3 c)
{
    return c.r * 255 * 255 + c.g * 255 + c.b;
}

void Bake(int iterations, int hemicubeSize)
{
    // Create framebuffer for rendering colors to
    unsigned int framebuffer, colorTex, depthStencil;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, hemicubeSize, hemicubeSize, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);

    glGenRenderbuffers(1, &depthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, hemicubeSize, hemicubeSize);
    glBindRenderbuffer(GL_RENDERBUFFER, 0); // unbind
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil);

    unsigned int DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, DrawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        cout << "ERROR: Framebuffer shat itself!" << endl;
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glViewport(0, 0, hemicubeSize, hemicubeSize);

    shader.use();
    shader.SetMatrix4x4("projection", glm::perspective(glm::radians(90.f), 1.f, 0.1f, 100.f));
    shader.SetInt("size", hemicubeSize);

    // Determine form factors
    int i = 0;
    for (Patch& pi : patches)
    {
        cout << "Calculating form factors for patch " << ++i << " of " << patches.size() << "..." << endl;
        for (int viewI = 0; viewI < 5; viewI++)
        {
            glClearColor(0.f, 0.f, 0.f, 0.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            // for side views, the bottom half ends up under the patch, so render top half only
            shader.SetBool("topHalf", viewI != 0);
            shader.SetMatrix4x4("view", pi.views[viewI]);

            for (Patch& pj : patches)
            {
                if (&pj == &pi) continue;
                glm::vec3 c = ToColor(pj.id);
                shader.SetVector3("albedoColor", glm::vec3(1, 1, 1));
                pj.mesh.Draw(shader);
            }

            glm::vec4* colors = new glm::vec4[hemicubeSize * hemicubeSize];
            glReadPixels(0, 0, hemicubeSize, hemicubeSize, GL_RGBA32F, GL_FLOAT, colors);

            // read the resulting pixels to get total form factor
            for (int j = 0; j < hemicubeSize; j++)
            {
                for (int i = 0; i < hemicubeSize; i++)
                {
                    // i,j = 0,0 should correspond to x,y = -1,-1, since pixel 0,0 is the bottom-left corner of the screen... probably
                    float x = (float)i / (float)hemicubeSize * 2.f - 1.f; // [-1,1]
                    float y = (float)j / (float)hemicubeSize * 2.f - 1.f; // [-1,1]

                    float dF = ((viewI == 0 ? 1.f : glm::max(y, 0.f)) / (float)(hemicubeSize * hemicubeSize)) /
                        (glm::pi<float>() * glm::pow((1.f + x * x + y * y), 2.f));
                    glm::vec4 c = colors[j * hemicubeSize + i];
                    pi.formFactors[FromColor(glm::vec3(c))] += dF * c.x;
                }
            }
            delete[] colors;
        }
        pi.finalColor = pi.emission;
    }

    // Send flux from each patch to each other patch
    for (int i = 0; i < iterations; i++)
    {
        cout << "Beginning flux output iteration " << i << "..." << endl;
        for (Patch& pi : patches)
        {
            for (auto& pair : pi.formFactors)
            {
                Patch& pj = patches[pair.first];
                pj.incident += pi.emission * pj.reflectance * pair.second * (pi.area / pj.area);
            }
        }
        // Set each patch to emit their resulting exident light (incident * reflectance) for next iteration.
        // also add this exident light to their final color.
        for (Patch& p : patches)
        {
            p.emission = p.incident;
            p.finalColor += p.incident;
            p.incident = glm::vec3(0); // reset for next iteration
        }
    }

    // cleanup
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &colorTex);
    glDeleteRenderbuffers(1, &depthStencil);
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
}

void init()
{
    stbi_set_flip_vertically_on_load(true);

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    oldT = glfwGetTime();

    // set up shader
    shader = Shader("../ABCore/Shaders/vertex.vert", "../ABCore/Shaders/frag_unlit.frag");

    // light patch
    glm::vec3 lightV[4] =
    {
        glm::vec3(0.9f, 2.97f, -4.1f),
        glm::vec3(-0.5f, 2.97f, -4.1f),
        glm::vec3(-0.5f, 2.97f, -5.5f),
        glm::vec3(0.9f, 2.97f, -5.5f)
    };
    patches.push_back(CreatePatch(lightV, glm::vec3(6)));

    // set up scene model
    GameObject* scene = Scene::Get().Add(GameObject("../Assets/cornell-box-holes2-subdivided2.obj", "Cornell Box"));
    scene->SetWorldTM({ 3, -2.5f, -2 }, glm::quat({ glm::pi<float>() / 2.f, glm::pi<float>(), glm::pi<float>() }));
    scene->GetMaterial().albedo = glm::vec3(1.f);

    // Patch generation
    glm::mat4 world = scene->GetWorldTM().GetMatrix();
    for (Mesh& m : scene->GetMeshes())
    {
        for (int i = 0; i < m.indices.size(); i += 6)
        {
            // every 6 indices = 0, 1, 2, 0, 2, 3, so just use 0, 1, 2, and 5 to get the 4 verts
            glm::vec3 verts[4] =
            {
                glm::vec3(world * glm::vec4(m.vertices[m.indices[i + 0]].Position, 1)),
                glm::vec3(world * glm::vec4(m.vertices[m.indices[i + 1]].Position, 1)),
                glm::vec3(world * glm::vec4(m.vertices[m.indices[i + 2]].Position, 1)),
                glm::vec3(world * glm::vec4(m.vertices[m.indices[i + 5]].Position, 1))
            };
            patches.push_back(CreatePatch(verts, glm::vec3(0)));
        }
    }
    Bake(4, 64);
}

void Tick()
{
    float newT = glfwGetTime();
    dt = newT - oldT;
    oldT = newT;

    // display FPS
    stringstream ss;
    ss << "Radiosity" << " [" << 1.f / dt << " FPS]";
    glfwSetWindowTitle(window, ss.str().c_str());

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
    shader.SetBool("topHalf", false);
    shader.SetMatrix4x4("world", glm::mat4());

    for (Patch& p : patches)
    {
        shader.SetVector3("albedoColor", p.finalColor);
        p.mesh.Draw(shader);
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
    window = glfwCreateWindow(width, height, "Radiosity", NULL, NULL);
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