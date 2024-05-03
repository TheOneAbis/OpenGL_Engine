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

#include <thread>

using namespace std;
using namespace AB;

// the window's width and height
GLFWwindow* window;
int width = 1000, height = 1000;

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
glm::vec3* colorData;
vector<thread> threads;

Transform camTM;

float aspect = (float)width / (float)height;
float fov = glm::pi<float>() / 2.f;
glm::vec3 ambient = glm::vec3(0.1f, 0.1f, 0.1f);
glm::vec3 screenCol = glm::vec3(0.25f, 0.61f, 1.f);
float LMax = 500.f;

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

    // set up scene models
    // mirror sphere
    GameObject* smallSphere = Scene::Get().Add(GameObject("../Assets/sphere.fbx", "Mirror Sphere"));
    smallSphere->SetWorldTM({ -1.f, -0.35f, -2.f }, glm::quat(), glm::vec3(0.5f));
    //GameObject* smallSphere = Scene::Get().Add(GameObject({ Mesh(0.5f) }, "Mirror Sphere"));
    //smallSphere->SetWorldTM({ -1.f, -0.35f, -2.f });

    smallSphere->GetMaterial().albedo = { 0.7f, 0.7f, 0.7f };
    smallSphere->GetMaterial().roughness = 0.f;
    smallSphere->GetMaterial().transmissive = 0.f;
    smallSphere->GetMaterial().reflectance = 0.75f;
    smallSphere->GetMaterial().diffuse = 0.25f;
    
    // glass sphere
    GameObject* bigSphere = Scene::Get().Add(GameObject("../Assets/sphere.fbx", "Glass Sphere"));
    bigSphere->SetWorldTM({ 0, 0, -1.5f }, glm::quat(), glm::vec3(0.75f));
    //GameObject* bigSphere = Scene::Get().Add(GameObject({Mesh(0.75f)}, "Glass Sphere"));
    //bigSphere->SetWorldTM({ 0, 0, -1.5f });

    bigSphere->GetMaterial().albedo = { 1, 1, 1 };
    bigSphere->GetMaterial().roughness = 0.8f;
    //bigSphere->GetMaterial().metallic = 0.5f;
    bigSphere->GetMaterial().transmissive = 0.8f;
    bigSphere->GetMaterial().reflectance = 0.01f;
    bigSphere->GetMaterial().diffuse = 0.075f;
    bigSphere->GetMaterial().refraction = 0.95f;

    // cone (bonus)
    /*GameObject* cone = Scene::Get().Add(GameObject("../Assets/cone.obj", "Cone"));
    cone->SetWorldTM({1.f, 0.3f, -3.f});
    cone->GetMaterial().albedo = { 0.3f, 1.f, 0.4f };
    cone->GetMaterial().roughness = 0.5f;*/
    
    // floor
    mFloor = Scene::Get().Add(GameObject({ Mesh(
        {
            { glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 0.f) },
            { glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 0.f) },
            { glm::vec3(1.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 1.f) },
            { glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 1.f) }
        }, 
        { 0, 1, 2, 0, 2, 3 }, 
        vector<Texture>()) }, "Floor"));
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
                vert.Normal = glm::normalize(glm::inverse(glm::transpose(glm::mat3(world))) * vert.Normal);
            }
        }
    }

    // construct KD-tree
    Scene::Get().CreateTree(1);

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

//  -- Cook-Torrence PBR BRDF functions -- \\
=============================================

// Fresnel value for non-metals
const float NONMETAL_F0 = 0.04f;
const float MIN_ROUGHNESS = 0.0000001f;

// Normal Distribution
float D_GGX(glm::vec3 n, glm::vec3 h, float roughness)
{
    float NdotH = glm::clamp(glm::dot(n, h), 0.f, 1.f);
    float NdotH2 = NdotH * NdotH;
    float a = roughness * roughness;
    float a2 = glm::max(a * a, MIN_ROUGHNESS);

    // ((n dot h)^2 * (a^2 - 1) + 1)
    float denomToSquare = NdotH2 * (a2 - 1) + 1;

    return a2 / (glm::pi<float>() * denomToSquare * denomToSquare);
}

// Fresnel term
glm::vec3 Fresnel(glm::vec3 v, glm::vec3 h, glm::vec3 f0)
{
    float VdotH = glm::clamp(glm::dot(v, h), 0.f, 1.f);
    return f0 + (1.f - f0) * glm::pow(1.f - VdotH, 5.f);
}

// Geometric Shadowing
float G_SchlickGGX(glm::vec3 n, glm::vec3 v, float roughness)
{
    float k = glm::pow(roughness + 1.f, 2.f) / 8.0f;
    float NdotV = glm::clamp(glm::dot(n, v), 0.f, 1.f);

    return 1 / (NdotV * (1 - k) + k);
}

// Cook-Torrance
// f(l,v) = D(h)F(v,h)G(l,v,h) / 4(n dot l)(n dot v)
glm::vec3 Microfacet(glm::vec3 n, glm::vec3 l, glm::vec3 v, float roughness, glm::vec3 f0, glm::vec3& F)
{
    glm::vec3 h = glm::normalize(v + l);

    // Run numerator functions
    float D = D_GGX(n, h, roughness);
    F = Fresnel(v, h, f0);
    float G = G_SchlickGGX(n, v, roughness) * G_SchlickGGX(n, l, roughness);

    glm::vec3 specularResult = (D * F * G) / 4.f;
    return specularResult * glm::max(glm::dot(n, l), 0.f);
}

glm::vec3 CookTorrence(glm::vec3 normal, glm::vec3 lightDir, glm::vec3 lightColor, glm::vec3 surfaceColor,
    glm::vec3 viewVec, float lightIntensity, float roughness, float metalness, glm::vec3 specColor)
{
    // Diffuse is still just N dot L
    float diffuse = glm::clamp(glm::dot(normal, -lightDir), 0.f, 1.f);

    // Get specular color and fresnel result
    glm::vec3 fresnel;
    glm::vec3 spec = Microfacet(normal, -lightDir, viewVec, roughness, specColor, fresnel);

    // Diffuse w/ energy conservation (also accounting for metals)
    glm::vec3 balancedDiff = diffuse * (1.f - fresnel) * (1.f - metalness);

    return (balancedDiff * surfaceColor + spec) * lightIntensity * lightColor;
}

//  -- Goofy basic Phong BRDF function -- \\
=============================================

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
    //glm::vec3 specularColor = glm::mix(glm::vec3(NONMETAL_F0, NONMETAL_F0, NONMETAL_F0), baseColor, m.metallic); // for cooktorrence
    float spec = 1 - glm::clamp(m.roughness, 0.f, 0.99f);

    glm::vec3 viewVector = glm::normalize(origin - hit.position);

    // Calculate lighting at the hit (metallic is for CookTorrence)
    glm::vec3 hitColor = ambient * baseColor * (1 - m.metallic);

    // Loop through the lights
    for (Light& l : lights)
    {
        glm::vec3 lightDir = l.Type == LIGHT_TYPE_DIRECTIONAL ? l.Direction * 500.f : hit.position - l.Position;
        bool attenuate = l.Type != LIGHT_TYPE_DIRECTIONAL;
        
        // shadow ray for each light; do lighting if no hit
        if (!Scene::Get().Raycast(hit.position, glm::normalize(-lightDir), nullptr, glm::length(lightDir)))
        {
            lightDir = glm::normalize(lightDir);
            glm::vec3 lightCol = Phong(hit.normal, lightDir, l.Color, baseColor, viewVector, spec, m.diffuse) * l.Intensity;
            //glm::vec3 lightCol = CookTorrence(hit.normal, lightDir, l.Color, baseColor, viewVector, l.Intensity, m.roughness, m.metallic, specularColor);

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

glm::vec3 Raytrace(glm::vec3 origin, glm::vec3 dir, int depth, int maxDepth, float incomingRefr)
{
    if (depth > maxDepth)
        return glm::vec3(0, 0, 0);
    
    RaycastHit hit;
    if (Scene::Get().Raycast(origin, dir, &hit))
    {
        // Local illumination and shadow casting
        glm::vec3 lightColor = LocalIlluminate(origin, hit);
        float refr = hit.gameObject->GetMaterial().refraction;

        // reflection
        float kr = hit.gameObject->GetMaterial().reflectance;
        if (kr > 0.001f)
        {
            lightColor += kr * Raytrace(hit.position, glm::reflect(dir, hit.normal), depth + 1, maxDepth, refr);
        }

        // refraction
        float kt = hit.gameObject->GetMaterial().transmissive;
        if (kt > 0.001f)
        {
            lightColor += kt * Raytrace(hit.position, glm::refract(dir, hit.normal, incomingRefr / refr), depth + 1, maxDepth, refr);
        }

        return lightColor;
    }
    else
    {
        return screenCol;
    }
}

inline float ToLuminance(glm::vec3 color)
{
    return 0.27f * color.r + 0.67f * color.g + 0.06f * color.b;
}

void CalculateRow(int y, float* rowNits, atomic<int>* counter)
{
    float totalNits = 0;
    for (int x = 0; x < width; x++)
    {
        // convert [0,1] range to [-1, 1]
        float xPercent = (float)x / (float)width * 2.f - 1.f;
        float yPercent = (float)y / (float)width * 2.f - 1.f;
        glm::vec3 dir = glm::normalize(glm::vec3(xPercent * fov * aspect, yPercent * fov, -1.f));

        glm::vec3 result = Raytrace(camTM.GetTranslation(), dir, 0, 5, 1);
        colorData[y * width + x] = result;

        // add log(l) to total
        float l = ToLuminance(result * LMax);
        totalNits += glm::log(FLT_EPSILON + l);
    }
    *rowNits = totalNits;
    int current = counter->load();
    counter->store(current + 1);
}

// called when the GL context need to be rendered
void display(void)
{
    colorData = new glm::vec3[width * height];

    glClearColor(0.25f, 0.61f, 1.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 invView = glm::inverse(glm::lookAt(camTM.GetTranslation(), camTM.GetTranslation() - camTM.GetForward(), glm::vec3(0.f, 1.f, 0.f)));
    float* rowNits = new float[height];
    float logavg = 0;
    
    // goofy multithreading to try and speed up this bs
    atomic<int> counter = 0;
    for (int y = 0; y < height; y++)
    {
        CalculateRow(y, &rowNits[y], &counter);
        //threads.push_back(thread(CalculateRow, y, &rowNits[y], Lmax, &counter));
    }
        
    while (counter.load() < height)
        continue;

    // calculate log-average luminance
    for (int i = 0; i < height; i++)
        logavg += rowNits[i];

    logavg /= width * height;

    trShader.use();
    trShader.SetFloat("LAvg", glm::exp(logavg));
    trShader.SetFloat("LMax", LMax);
    trShader.SetFloat("LdMax", 500.f);
    trShader.SetFloat("KeyValue", 0.72f);
    trShader.SetFloat("bias", 0.85f);
    trShader.SetUint("operator", 0);

    glBindTexture(GL_TEXTURE_2D, viewportTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, colorData);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    tri.Draw(trShader);

    delete[] rowNits;
    delete[] colorData;
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