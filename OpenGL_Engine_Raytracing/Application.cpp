//
//  Application.cpp
//
//  Assignment-specific code.
//
//  Created by Warren R. Carithers on 2019/09/09.
//  Based on earlier versions created by Joe Geigel and Warren R. Carithers
//  Copyright 2019 Rochester Institute of Technology. All rights reserved.
//
//  This file should not be modified by students.
//

#include <cstdlib>
#include <iostream>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

//
// GLEW and GLFW header files also pull in the OpenGL definitions
//
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "Types.h"
#include "Utils.h"
#include "ShaderSetup.h"
#include "Buffers.h"
#include "Models.h"

#include "Application.h"

using namespace std;

//
// PRIVATE GLOBALS
//

// names of our GLSL shader files (we assume GLSL 1.50)
static const char *vshader = "v150.vert";
static const char *fshader = "v150.frag";

// buffers for our shapes
static BufferSet shape1, shape2;

// shader program handle
static GLuint program;

// our VAO
static GLuint vao;

// do we need to do a display() call?
static bool updateDisplay = true;

// click counter for color selection
static int counter = 0;

//
// PUBLIC GLOBALS
//

//
// Drawing-related variables
//

// dimensions of the drawing window
int w_width  = 512;
int w_height = 512;

// drawing window title
const char *w_title = "Hello, OpenGL!";

// GL context we're using (we assume 3.0, for GLSL 1.30)
int gl_maj = 3;
int gl_min = 0;

// our GLFWwindow
GLFWwindow *w_window;

//
// PRIVATE FUNCTIONS
//

///
/// Create the shapes we'll display
///
static void createImage( void )
{
    // create the first shape
    createObject( 1, shape1 );

    // now, the second shape
    createObject( 2, shape2 );
}

//
// Event callback routines for this assignment
//

///
/// Handle keyboard input
///
static void keyboard( GLFWwindow *window, int key, int scan,
                      int action, int mods )
{
    // only react to "press" events
    if( action != GLFW_PRESS ) {
        return;
    }

    switch( key ) {
    case GLFW_KEY_H:    // help message
        cout << "  Key(s)             Action" << endl;
        cout << "=========   =======================" << endl;
        cout << "   1        Select the first image" << endl;
        cout << "   2        Select the second image" << endl;
        cout << "ESC, q, Q   Terminate the program" << endl;
        cout << " h, H       Print this message" << endl;
        break;


    case GLFW_KEY_1:
        counter = 0;
        break;

    case GLFW_KEY_2:
        counter = 1;
        break;

    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose( window, 1 );
        break;
    }
}

///
/// Handle mouse clicks
///
static void mouse( GLFWwindow *window, int button, int action, int mods )
{
    if( button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS ) {
        counter = ~counter;
    }
}

///
/// Display the current image
///
static void display( void )
{
    int nv;

    // clear the frame buffer
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // ensure we have selected the shader program
    glUseProgram( program );

    // set up our scale factors for normalization
    // GLuint sf = glGetUniformLocation( program, "sf" );
    // glUniform2f( sf, 2.0f / (w_width - 1.0f), 2.0f / (w_height - 1.0f) );

    // bind the vertex and element buffers
    if( (counter & 1) == 0 ) {
        shape1.selectBuffers( program, "vPosition", "vColor", NULL, NULL );
        nv = shape1.numElements;
    } else {
        shape2.selectBuffers( program, "vPosition", "vColor", NULL, NULL );
        nv = shape2.numElements;
    }

    // draw the shapes
    glDrawElements( GL_TRIANGLES, nv, GL_UNSIGNED_INT, (void *)0 );
}

///
/// OpenGL initialization
///
static bool init( void )
{
    // Check the OpenGL version - need at least 3.2
    if( gl_maj < 3 || gl_min < 2 ) {
        cerr << "Error - cannot use OpenGL 3.2" << endl;
        return( false );
    }

    // Load shaders and use the resulting shader program
    ShaderError error;
    program = shaderSetup( vshader, fshader, &error );
    if( !program ) {
        cerr << "Error setting up shaders - "
             << errorString(error) << endl;
        return( false );
    }

    // need a VAO if we're using a core context;
    // doesn't hurt even if we're not using one
    glGenVertexArrays( 1, &vao );
    //glBindVertexArray( vao );

    // OpenGL state initialization
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );
    glClearColor( 0.0, 0.0, 0.0, 1.0 );
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

    // create the geometry for our shapes.
    createImage();

    // register our callbacks

    // key callback - for when we only care about the physical key
    glfwSetKeyCallback( w_window, keyboard );

    // mouse click handler
    glfwSetMouseButtonCallback( w_window, mouse );

    return( true );
}

//
// PUBLIC FUNCTIONS
//

///
/// application-specific processing
///
void application( int argc, char *argv[] )
{
    // keep the compiler happy
    (void) argc;
    (void) argv;

    // set up the objects and the scene
    if( !init() ) {
        return;
    }

    // loop until it's time to quit
    while( !glfwWindowShouldClose(w_window) ) {
        display();
        glfwSwapBuffers( w_window );
        glfwWaitEvents();
    }
}
