#include "Input.h"

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
	return keymap[key] && prevkeymap[key];
}
bool Input::KeyReleased(const char key)
{
	return !keymap[key] && prevkeymap[key];
}
unordered_map<char, bool>& Input::GetKeymap()
{
	return keymap;
}

void ProcessKeyInput(unsigned char key, int xMouse, int yMouse)
{
	Input::Get().GetKeymap()[key] = true;
}
void ProcessKeyUpInput(unsigned char key, int xMouse, int yMouse)
{
	Input::Get().GetKeymap()[key] = false;
}

void Input::Init()
{
	glutKeyboardFunc(ProcessKeyInput);
	glutKeyboardUpFunc(ProcessKeyUpInput);
}
void Input::Update()
{
	prevkeymap = keymap;
}

Input::~Input() {}