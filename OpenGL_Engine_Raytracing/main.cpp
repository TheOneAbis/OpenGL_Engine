#pragma comment(lib, "ABCore.lib")

#include <GL/glew.h>

// GLM math library
#define GLM_FORCE_RADIANS
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include <GLFW/glfw3.h>

#include <iostream>
#include <ABCore/Scene.h>

using namespace std;
using namespace AB;

// the window's width and height
GLFWwindow* window;
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
Shader trShader;
Mesh tri;
unsigned int viewportTex;
glm::vec4* colorData;

Transform camTM;

float aspect = (float)width / (float)height;
float fov = glm::pi<float>() / 2.f;
glm::vec3 ambient = glm::vec3(0.1f, 0.1f, 0.1f);
glm::vec3 screenCol = glm::vec3(0.25f, 0.61f, 1.f);
int recursionDepth = 4;

GameObject* mFloor;

void init()
{
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    trShader = Shader("../ABCore/Shaders/vert_screen.vert", "../ABCore/Shaders/frag_TR.frag");
    tri = Mesh(
        {
            { glm::vec3(-1, 1, 0), glm::vec3(), glm::vec2() },
            { glm::vec3(-1, -3, 0), glm::vec3(), glm::vec2() },
            { glm::vec3(3, 1, 0), glm::vec3(), glm::vec2() }
        },
        { 0, 1, 2 }, {});
    colorData = new glm::vec4[width * height];

    // set up scene models
    // mirror sphere
    GameObject* smallSphere = Scene::Get().Add(GameObject("../Assets/sphere.fbx"));
    smallSphere->SetWorldTM({-1.f, -0.35f, -2.f}, glm::quat(), {0.5f, 0.5f, 0.5f});
    smallSphere->GetMaterial().albedo = { 0.7f, 0.7f, 0.7f };
    smallSphere->GetMaterial().roughness = 0.f;
    smallSphere->GetMaterial().transmissive = 0.f;
    smallSphere->GetMaterial().reflectance = 0.75f;
    smallSphere->GetMaterial().diffuse = 0.25f;
    
    // glass sphere
    GameObject* bigSphere = Scene::Get().Add(GameObject("../Assets/sphere.fbx"));
    bigSphere->SetWorldTM({ 0, 0, -1.5f }, glm::quat(), {.75f, .75f, .75f});
    bigSphere->GetMaterial().albedo = { 1, 1, 1 };
    bigSphere->GetMaterial().roughness = 0.8f;
    bigSphere->GetMaterial().metallic = 0.5f;
    bigSphere->GetMaterial().transmissive = 0.8f;
    bigSphere->GetMaterial().reflectance = 0.01f;
    bigSphere->GetMaterial().diffuse = 0.075f;
    bigSphere->GetMaterial().refraction = 0.95f;
    
    mFloor = Scene::Get().Add(GameObject({ Mesh(
        {
            { glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 0.f) },
            { glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 0.f) },
            { glm::vec3(1.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 1.f) },
            { glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 1.f) }
        }, 
        { 0, 1, 2, 0, 2, 3 }, 
        vector<Texture>()) }));
    mFloor->SetWorldTM(Transform({-2, -1, 0}, glm::quat(), {3, 1, 5}));
    mFloor->GetMaterial().albedo = { 0.2f, 0.3f, 0.9f };

    // set up light
    Light dirLight = {};
    dirLight.Type = LIGHT_TYPE_DIRECTIONAL;
    dirLight.Direction = glm::normalize(glm::vec3(-0.3f, -1.f, -0.5f));
    dirLight.Color = glm::vec3(1.f, 1.f, 1.f);
    dirLight.Intensity = 1.f;
    lights.push_back(dirLight);

    // change all verts to world space
    for (auto& obj : Scene::Get().GetAllObjects())
    {
        glm::mat4 world = obj.GetWorldTM().GetMatrix();
        for (auto& mesh : obj.GetMeshes())
        {
            for (auto& vert : mesh.vertices)
            {
                vert.Position = glm::vec3(world * glm::vec4(vert.Position, 1));
                vert.Normal = glm::inverse(glm::transpose(glm::mat3(world))) * vert.Normal;
            }
        }
    }

    glGenTextures(1, &viewportTex);
}

glm::vec3 GetFloorColor(glm::vec3 worldPos)
{
    glm::vec3 color = glm::vec3(1, 1, 0);

    glm::ivec3 scaledPos = glm::abs(glm::ivec3(worldPos * 4.f) % 2);
    glm::ivec2 scaledPosxz = { scaledPos.x, scaledPos.z };
    if (scaledPosxz == glm::ivec2(0, 0) || scaledPosxz == glm::ivec2(1, 1))
        color.y = 0;
    if (worldPos.x < 0)
        color.y = 1 - color.y;

    return color;
}

glm::vec3 Phong(glm::vec3 normal, glm::vec3 lightDir, glm::vec3 lightColor, glm::vec3 colorTint, glm::vec3 viewVec, float specScale, float diffuseScale)
{
    // Calculate diffuse and specular values
    float diffuse = glm::clamp(glm::dot(normal, -lightDir), 0.f, 1.f) * diffuseScale;
    float spec = glm::pow(glm::clamp(glm::dot(glm::reflect(lightDir, normal), viewVec), 0.f, 1.f), 20.f) * specScale;

    // Cut the specular if the diffuse contribution is zero
    spec *= diffuse < 0.001f ? 0.f : 1.f;

    return lightColor * colorTint * (diffuse + spec);
}

glm::vec3 LocalIlluminate(glm::vec3 origin, RaycastHit hit)
{
    Material& m = hit.gameObject->GetMaterial();

    glm::vec3 baseColor = hit.gameObject == mFloor ? GetFloorColor(hit.position) : m.albedo;
    float spec = 1 - glm::clamp(m.roughness, 0.f, 0.99f);
    glm::vec3 viewVector = glm::normalize(origin - hit.position);

    // Calculate lighting at the hit
    glm::vec3 hitColor = ambient * baseColor;

    // Loop through the lights
    for (Light& l : lights)
    {
        glm::vec3 lightDir = l.Type == LIGHT_TYPE_DIRECTIONAL ? l.Direction * 500.f : hit.position - l.Position;
        bool attenuate = l.Type != LIGHT_TYPE_DIRECTIONAL;

        // shadow ray for each light; do lighting if no hit
        if (!Scene::Get().Raycast(hit.position, -lightDir, glm::length(lightDir)))
        {
            lightDir = glm::normalize(lightDir);
            glm::vec3 lightCol = Phong(hit.normal, lightDir, l.Color, baseColor, viewVector, spec, m.diffuse) * l.Intensity;

            // attenuate the light if not directional
            if (attenuate)
            {
                float dist = glm::distance(l.Position, hit.position);
                lightCol *= glm::pow(glm::clamp(1.f - (dist * dist / (l.Range * l.Range)), 0.f, 1.f), 2.f);
            }
            hitColor += lightCol;
        }
    }
    return hitColor;
}

glm::vec3 Raytrace(glm::vec3 origin, glm::vec3 dir, int depth, float incomingRefr)
{
    if (depth > recursionDepth)
        return glm::vec3(0, 0, 0);
    
    RaycastHit hit;
    if (Scene::Get().Raycast(origin, dir, hit))
    {
        // Local illumination and shadow casting
        glm::vec3 lightColor = LocalIlluminate(origin, hit);

        // reflection
        float kr = hit.gameObject->GetMaterial().reflectance;
        if (kr > 0.001f)
        {
            lightColor += kr * Raytrace(hit.position, glm::reflect(dir, hit.normal), depth + 1, hit.gameObject->GetMaterial().refraction);
        }

        // refraction
        float kt = hit.gameObject->GetMaterial().transmissive;
        if (kt > 0.001f)
        {
            float refr = hit.gameObject->GetMaterial().refraction;
            lightColor += kt * Raytrace(hit.position, glm::refract(dir, hit.normal, incomingRefr / refr), depth + 1, refr);
        }

        return lightColor;
    }
    else
    {
        return screenCol;
    }
}

// called when the GL context need to be rendered
void display(void)
{
    glClearColor(0.25f, 0.61f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 invView = glm::inverse(glm::lookAt(camTM.GetTranslation(), camTM.GetTranslation() - camTM.GetForward(), glm::vec3(0.f, 1.f, 0.f)));
    glm::vec3 avgIntensity = {};

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // convert [0,1] range to [-1, 1]
            /*float xPercent = (float)x / ((float)width * 2.f - 1.f);
            float yPercent = (float)y / ((float)width * 2.f - 1.f);
            glm::vec4 viewSpaceDir = glm::vec4(glm::normalize(glm::vec3(xPercent * fov * aspect, yPercent * fov, -1.f)), 1.f);
            glm::vec3 dir = glm::normalize(glm::vec3(invView * viewSpaceDir) - camTM.GetTranslation());

            glm::vec4 result = glm::vec4(Raytrace(camTM.GetTranslation(), dir, 0, 1), 1);*/

            glm::vec4 result = { 0, 1, 1, 1};

            colorData[y * width + x] = result;
            avgIntensity += glm::vec3(result);
        }
    }
    
    glBindTexture(GL_TEXTURE_2D, viewportTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_FLOAT, colorData);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, viewportTex);

    avgIntensity /= (float)(width * height);

    trShader.use();
    trShader.SetVector3("avgIntensity", avgIntensity);
    
    tri.Draw(trShader);
}

// called when window is first created or when window is resized
void reshape(GLFWwindow* window, int w, int h)
{
    width = w;
    height = h;

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
    window = glfwCreateWindow(width, height, "Ray Tracer", NULL, NULL);
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

    // Main Loop
    display();
    glfwSwapBuffers(window);
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}