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
Mesh tri;
GameObject smallSphere, bigSphere, cone, mFloor;
std::vector<GameObject> gameObjects;

Transform camTM;
glm::vec2 camEulers;
float dt, oldT;

unsigned int vertBuffer, indexBuffer, worldMatrices;
unsigned int vertTex, indexTex, worldMatricesTex;
int indexCount = 0;

void init()
{
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    oldT = glutGet(GLUT_ELAPSED_TIME) / 1000.f;

    // set up shader
    shader = Shader("../Shaders/vert_screen.vert", "../Shaders/frag_raytracing.frag");
    tri = Mesh(
        {
            { glm::vec3(-1, 1, 0), glm::vec3(), glm::vec2() },
            { glm::vec3(-1, -3, 0), glm::vec3(), glm::vec2() },
            { glm::vec3(3, 1, 0), glm::vec3(), glm::vec2() }
        }, 
        {0, 1, 2}, {});

    // set up scene models
    smallSphere = GameObject("Assets/sphere.fbx");
    smallSphere.SetWorldTM({-1.f, -0.5f, -2.f}, glm::quat(), {0.5f, 0.5f, 0.5f});
    smallSphere.GetMaterial().albedo = { 0.3f, 0.3f, 0.1f };

    bigSphere = GameObject("Assets/sphere.fbx");
    bigSphere.SetWorldTM({ 0, 0.0f, -1.5f }, glm::quat(), {.75f, .75f, .75f});
    auto* mat = &bigSphere.GetMaterial();
    mat->albedo = { 0.5f, 0.1f, 0.5f };
    mat->metallic = 0.5f;
   
    cone = GameObject("Assets/cone.obj");
    cone.SetWorldTM({ 2.f, 0.5f, -2.f }, glm::quat(), {1.f, 1.f, 1.f});
    mat = &cone.GetMaterial();
    mat->albedo = { 0.8f, 0.3f, 0.1f };
    mat->specular = 0.2f;
    
    mFloor = GameObject({ Mesh(
        {
            { glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 0.f) },
            { glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 0.f) },
            { glm::vec3(1.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(1.f, 1.f) },
            { glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f), glm::vec2(0.f, 1.f) }
        }, 
        { 0, 1, 2, 0, 2, 3 }, 
        vector<Texture>()) });
    mFloor.SetWorldTM(Transform({-2, -1, 0}, glm::quat(), {3, 1, 5}));
    mFloor.GetMaterial().albedo = { -0.2f, 0.3f, 0.9f };

    gameObjects = { bigSphere, smallSphere, cone, mFloor };

    // set up lights
    Light point = {};
    point.Type = LIGHT_TYPE_POINT;
    point.Color = glm::vec3(0.f, 1.f, 1.f);
    point.Intensity = 10.f;
    point.Position = glm::vec3(-2, 0, 0);
    point.Range = 3.f;
    //lights.push_back(point);

    Light dirLight = {};
    dirLight.Type = LIGHT_TYPE_DIRECTIONAL;
    dirLight.Direction = glm::normalize(glm::vec3(-0.3f, -1.f, -0.5f));
    dirLight.Color = glm::vec3(1.f, 1.f, 1.f);
    dirLight.Intensity = 0.5f;
    lights.push_back(dirLight);

    // create the giant vertex buffer
    vector<glm::vec4> vertData;
    vector<unsigned int> indexData;
    vector<glm::mat4> worldMatData;
    for (auto& obj : gameObjects)
    {
        worldMatData.push_back(obj.GetWorldTM().GetMatrix());

        for (auto& mesh : obj.GetMeshes())
        {
            for (auto& i : mesh.indices)
            {
                indexData.push_back(i + (vertData.size() / 3));
            }
            
            for (auto& vert : mesh.vertices)
            {
                // store which world matrix this vert should use in the position's w coord
                vertData.push_back(glm::vec4(vert.Position, (float)worldMatData.size() - 1.f));
                vertData.push_back(glm::vec4(vert.Normal, 1));
                vertData.push_back(glm::vec4(vert.TexCoord, 0, 1));
            }
        }
    }
    
    // generate the buffers
    glGenBuffers(1, &vertBuffer);
    glGenBuffers(1, &indexBuffer);
    glGenBuffers(1, &worldMatrices);

    glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * vertData.size(), &vertData[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int) * indexData.size(), &indexData[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, worldMatrices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * worldMatData.size(), &worldMatData[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // gen texture buffers and attach the above buffer objects to them
    glGenTextures(1, &vertTex);
    glGenTextures(1, &indexTex);
    glGenTextures(1, &worldMatricesTex);

    glBindTexture(GL_TEXTURE_BUFFER, vertTex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, vertBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, indexTex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, indexBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, worldMatricesTex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, worldMatrices);
    
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    // set the sampler buffers
    indexCount = indexData.size();
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

    shader.SetMatrix4x4("view", glm::lookAt(camTM.GetTranslation(), camTM.GetTranslation() - camTM.GetForward(), glm::vec3(0.f, 1.f, 0.f)));
    shader.SetFloat("aspectRatio", (float)width / (float)height);
    shader.SetFloat("cameraFOV", glm::pi<float>() / 2.f);
    shader.SetVector3("cameraPos", camTM.GetTranslation());
    shader.SetInt("indexCount", indexCount);
    shader.SetVector3("ambient", glm::vec3(0.1f, 0.1f, 0.1f));
    
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

    // bind the texture buffers to their respective texture units as defined in the frag shader
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, vertTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, indexTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, worldMatricesTex);

    glActiveTexture(GL_TEXTURE0);

    tri.Draw(shader);

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