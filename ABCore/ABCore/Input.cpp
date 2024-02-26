#include "Input.h"

#include <glm/glm.hpp>

#ifdef __APPLE__
#include <GLUT/glut.h> // include glut for Mac
#else
#include <GL/freeglut.h> //include glut for Windows
#endif

using namespace std;
using namespace AB;

Input* Input::instance;

bool Input::KeyDown(const char key)
{
	return keymap[key];
}
bool Input::KeyPressed(const char key)
{
	return keymap[key] && !prevkeymap[key];
}
bool Input::KeyReleased(const char key)
{
	return !keymap[key] && prevkeymap[key];
}
bool Input::MouseButtonDown(int button)
{
	return mousemap[button];
}
bool Input::MouseButtonPressed(int button)
{
	return mousemap[button] && !prevmousemap[button];
}
bool Input::MouseButtonReleased(int button)
{
	return !mousemap[button] && prevmousemap[button];
}

unordered_map<char, bool>& Input::GetKeyMap()
{
	return keymap;
}
std::unordered_map<int, bool>& Input::GetMouseMap()
{
	return mousemap;
}
glm::vec2& Input::GetMousePos()
{
	return *mousePos;
}
glm::vec2& Input::GetLastMousePos()
{
	return *prevMousePos;
}
glm::vec2 Input::GetMouseDelta()
{
	return *mousePos - *prevMousePos;
}

void ProcessKeyInput(unsigned char key, int xMouse, int yMouse)
{
	Input::Get().GetKeyMap()[key] = true;
}
void ProcessKeyUpInput(unsigned char key, int xMouse, int yMouse)
{
	Input::Get().GetKeyMap()[key] = false;
}
void ProcessMouseInput(int button, int state, int x, int y)
{
	Input::Get().GetMouseMap()[button] = !state;
}
void ProcessMouseMotion(int x, int y)
{
	Input::Get().GetMousePos() = glm::vec2(x, y);
}

void Input::Init()
{
	mousePos = new glm::vec2();
	prevMousePos = new glm::vec2();

	glutKeyboardFunc(ProcessKeyInput);
	glutKeyboardUpFunc(ProcessKeyUpInput);
	glutMouseFunc(ProcessMouseInput);
	glutMotionFunc(ProcessMouseMotion);
	glutPassiveMotionFunc(ProcessMouseMotion);
}
void Input::Update()
{
	prevkeymap = keymap;
	prevmousemap = mousemap;
	*prevMousePos = *mousePos;
}

Input::~Input() 
{
	delete mousePos;
	delete prevMousePos;
}