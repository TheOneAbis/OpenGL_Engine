// link in my personal library
#pragma comment(lib, "ABCore.lib")

#include <GL/glew.h>

// GLM math library
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef __APPLE__
#include <GLUT/glut.h> // include glut for Mac
#else
#include <GL/freeglut.h> //include glut for Windows
#endif

#include <ABCore/GameObject.h>
#include <ABCore/Input.h>

#define PI 3.14159f

using namespace std;
using namespace AB;

// the window's width and height
glm::vec2 windowSize(1280, 720);
float viewScale = 320.f;

// the body parts (wrap game objects to save which ones were selected as the user traverses down or up the hierarchy)
struct GameObjectSelection
{
    GameObject gameObject;
    int selectedChild = 0;
};
vector<GameObjectSelection> gameObjects;
GameObject* selected = nullptr;
int treeDepth = 0;
float oldT = 0;

Shader shader;

void init()
{
    oldT = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
    shader = Shader("../ABCore/Shaders/vert2d.vert", "../ABCore/Shaders/frag2d.frag");

    // creating the rectangle to be reused for all drawing
    vector<unsigned int> indices = { 0, 1, 0, 1, 2, 1, 2, 3, 2, 3, 0, 3 };
    Mesh torsoMesh(
        {
            { glm::vec3(-0.35f, 0.f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(-0.35f, 0.5f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(0.35f, 0.5f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(0.35f, 0.f, 0.f), glm::vec3(), glm::vec2() }
        },
        indices, vector<Texture>());
    Mesh chestMesh(
        {
            { glm::vec3(-0.5f, 0.f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(-0.5f, 0.5f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(0.5f, 0.5f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(0.5f, 0.f, 0.f), glm::vec3(), glm::vec2() }
        },
        indices, vector<Texture>());
    Mesh neckMesh(
        {
            { glm::vec3(-0.1f, 0.f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(-0.1f, 0.25f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(0.1f, 0.25f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(0.1f, 0.f, 0.f), glm::vec3(), glm::vec2() }
        },
        indices, vector<Texture>());
    Mesh limbMesh(
        {
            { glm::vec3(-0.15f, 0.f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(-0.15f, 0.4f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(0.15f, 0.4f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(0.15f, 0.f, 0.f), glm::vec3(), glm::vec2() }
        },
        indices, vector<Texture>());
    Mesh handMesh(
        {
            { glm::vec3(-0.15f, 0.f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(-0.25f, 0.3f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(0.25f, 0.3f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(0.15f, 0.f, 0.f), glm::vec3(), glm::vec2() }
        },
        indices, vector<Texture>());
    Mesh headMesh(
        {
            { glm::vec3(-0.15f, 0.f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(-0.25f, 0.2f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(-0.15f, 0.3f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(0.15f, 0.3f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(0.25f, 0.2f, 0.f), glm::vec3(), glm::vec2() },
            { glm::vec3(0.15f, 0.f, 0.f), glm::vec3(), glm::vec2() }
        },
        { 0, 1, 0, 1, 2, 1, 2, 3, 2, 3, 4, 3, 4, 5, 4, 5, 0, 5 }, vector<Texture>());

    // creating all the body parts
    gameObjects.reserve(16); // pre-allocate, we don't want shit being moved around in memory while adding stuff

    gameObjects.push_back({ GameObject({ torsoMesh }, "Torso", Transform({ 0.f, 0.f, 0.f }, glm::quat(), {1, 1, 1}))}); // 0
    gameObjects.push_back({ GameObject({ chestMesh }, "Chest", Transform({ 0.f, 0.5f, 0.f }, glm::quat(), {1, 1, 1}), &gameObjects[0].gameObject) }); // 1 
    gameObjects.push_back({ GameObject({ limbMesh }, "Left Thigh", Transform({ -0.25f, 0.f, 0.f }, glm::quat({0 ,0, PI}), {1, 1, 1}), &gameObjects[0].gameObject) }); // 2
    gameObjects.push_back({ GameObject({ limbMesh }, "Right Thigh", Transform({ 0.25f, 0.f, 0.f }, glm::quat({0, 0, PI}), {1, 1, 1}), &gameObjects[0].gameObject) }); // 3
    gameObjects.push_back({ GameObject({ neckMesh }, "Neck", Transform({ 0.f, 0.5f, 0.f }, glm::quat(), {1, 1, 1}), &gameObjects[1].gameObject) }); // 4
    gameObjects.push_back({ GameObject({ limbMesh }, "Left Shoulder", Transform({ -0.5f, 0.25f, 0.f }, glm::quat({0, 0, PI / 2.f}), {1, 1, 1}), &gameObjects[1].gameObject) }); // 5
    gameObjects.push_back({ GameObject({ limbMesh }, "Right Shoulder", Transform({ 0.5f, 0.25f, 0.f }, glm::quat({0, 0, -PI / 2.f}), {1, 1, 1}), &gameObjects[1].gameObject) }); // 6
    gameObjects.push_back({ GameObject({ limbMesh }, "Left Leg", Transform({ 0.f, 0.4f, 0.f }, glm::quat(), {1, 1, 1}), &gameObjects[2].gameObject) }); // 7
    gameObjects.push_back({ GameObject({ limbMesh }, "Right Leg", Transform({ 0.f, 0.4f, 0.f }, glm::quat(), {1, 1, 1}), &gameObjects[3].gameObject) }); // 8
    gameObjects.push_back({ GameObject({ headMesh }, "Head", Transform({ 0.f, 0.25f, 0.f }, glm::quat(), {1, 1, 1}), &gameObjects[4].gameObject) }); // 9
    gameObjects.push_back({ GameObject({ limbMesh }, "Left Forearm", Transform({ 0.f, 0.4f, 0.f }, glm::quat(), {1, 1, 1}), &gameObjects[5].gameObject) }); // 10
    gameObjects.push_back({ GameObject({ limbMesh }, "Right Forearm", Transform({ 0.f, 0.4f, 0.f }, glm::quat(), {1, 1, 1}), &gameObjects[6].gameObject) }); // 11
    gameObjects.push_back({ GameObject({ handMesh }, "Left Foot", Transform({ 0.f, 0.4f, 0.f }, glm::quat(), {1, 1, 1}), &gameObjects[7].gameObject) }); // 12
    gameObjects.push_back({ GameObject({ handMesh }, "Right Foot", Transform({ 0.f, 0.4f, 0.f }, glm::quat(), {1, 1, 1}), &gameObjects[8].gameObject) }); // 13
    gameObjects.push_back({ GameObject({ handMesh }, "Left Hand", Transform({ 0.f, 0.4f, 0.f }, glm::quat(), {1, 1, 1}), &gameObjects[10].gameObject) }); // 14
    gameObjects.push_back({ GameObject({ handMesh }, "Right Hand", Transform({ 0.f, 0.4f, 0.f }, glm::quat(), {1, 1, 1}), &gameObjects[11].gameObject) }); // 15

    selected = &gameObjects[0].gameObject;
}

void tick()
{
    float newT = (glutGet(GLUT_ELAPSED_TIME) / 1000.f);
    float dt = newT - oldT;
    oldT = newT;

    //! A/D - cycle children
    Transform tm = selected->GetLocalTM();
    glm::vec3 eulers = glm::eulerAngles(tm.GetRotation());

    Input& input = Input::Get();
    if (input.KeyDown('a'))
    {
        eulers.z += (PI / 3.f) * dt;
        tm.SetRotationEulersXYZ(eulers);
        selected->SetLocalTM(tm);
    }
    else if (input.KeyDown('d'))
    {
        eulers.z -= (PI / 3.f) * dt;
        tm.SetRotationEulersXYZ(eulers);
        selected->SetLocalTM(tm);
    }
    else if (input.KeyPressed(27))
    {
        exit(0);
    }

    Input::Get().Update();
    glutPostRedisplay();
}

// called when the GL context need to be rendered
void display(void)
{
    // clear the screen to white, which is the background color
    glClearColor(1, 1, 1, 0);

    // clear the buffer stored for drawing
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    shader.use();
    shader.SetMatrix4x4("projection", glm::mat4(glm::ortho(-windowSize.x / viewScale, windowSize.x / viewScale, -windowSize.y / viewScale, windowSize.y / viewScale, -1000.f, 1000.f)));

    // draw all the game objects
    for (auto& selection : gameObjects)
    {
        selection.gameObject.GetMaterial().albedo = glm::vec3(0, 0, 0);
        selection.gameObject.Draw(shader);
    }
        
    // redraw the selected gameobject with a different color
    selected->GetMaterial().albedo = glm::vec3(1, 0, 0);
    selected->Draw(shader);

    glutSwapBuffers();
}

// called when window is first created or when window is resized
void reshape(int w, int h)
{
    // update screen dimensions
    windowSize = glm::vec2(w, h);
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glutPostRedisplay();
}

//! up - select parent
//! down - select child
void processSpecialInput(int key, int x, int y)
{
    // Reinterpret the pointer to be able to access the saved child index for this game object.
    // because the game object is the first member of the GameObjectSelection struct, 
    // I can do this without it shitting itself
    GameObjectSelection* parentSelection = nullptr;
    if (selected->GetParent())
        parentSelection = reinterpret_cast<GameObjectSelection*>(selected->GetParent());

    switch (key)
    {
    case GLUT_KEY_UP:
        if (selected->GetParent())
        {
            selected = selected->GetParent();
        }
        break;
    case GLUT_KEY_DOWN:
        if (!selected->GetChildren().empty())
        {
            selected = selected->GetChild(reinterpret_cast<GameObjectSelection*>(selected)->selectedChild);
        }
        break;
    case GLUT_KEY_RIGHT:
        if (parentSelection)
        {
            GameObject* parent = selected->GetParent();
            GameObject* prev = parent->GetChild(parentSelection->selectedChild - 1);
            if (prev)
            {
                parentSelection->selectedChild--;
                selected = prev;
            }
            else
            {
                parentSelection->selectedChild = parent->GetChildren().size() - 1;
                selected = parent->GetChild(parentSelection->selectedChild);
            }
        }
        break;
    case GLUT_KEY_LEFT:
        if (parentSelection)
        {
            GameObject* parent = selected->GetParent();
            GameObject* next = parent->GetChild(parentSelection->selectedChild + 1);
            if (next)
            {
                parentSelection->selectedChild++;
                selected = next;
            }
            else
            {
                parentSelection->selectedChild = 0;
                selected = parent->GetChild(parentSelection->selectedChild);
            }
        }
        break;
    }
    
    glutPostRedisplay();
}

int main(int argc, char* argv[])
{
    //initialize GLUT
    glutInit(&argc, argv);

    // double bufferred
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    //set the initial window size
    glutInitWindowSize((int)windowSize.x, (int)windowSize.y);

    // create the window with a title
    glutCreateWindow("OpenGL Transformations");

    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        // error occurred
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    /* --- register callbacks with GLUT --- */

    glutReshapeFunc(reshape);

    glutDisplayFunc(display);
    glutIdleFunc(tick);

    // Processing input
    Input::Get().Init();
    glutSpecialFunc(processSpecialInput);

    init();

    //start the glut main loop
    glutMainLoop();

    return 0;
}