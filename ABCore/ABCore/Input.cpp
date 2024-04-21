#include "Input.h"

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

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

unordered_map<int, bool>& Input::GetKeyMap()
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

void ProcessKeyInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Input::Get().GetKeyMap()[key] = action != 0;
}
void ProcessMouseInput(GLFWwindow* window, int button, int action, int mods)
{
	Input::Get().GetMouseMap()[button] = action == 1;
}
void ProcessMouseMotion(GLFWwindow* window, double x, double y)
{
	Input::Get().GetMousePos() = glm::vec2(x, y);
}

void Input::Init(GLFWwindow* window)
{
	mousePos = new glm::vec2();
	prevMousePos = new glm::vec2();
	glfwSetKeyCallback(window, ProcessKeyInput);
	glfwSetCursorPosCallback(window, ProcessMouseMotion);
	glfwSetMouseButtonCallback(window, ProcessMouseInput);
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