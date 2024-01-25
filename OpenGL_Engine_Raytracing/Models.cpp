//
//  Models.cpp
//
//  Assignment-specific code for objects.
//
//  Created by Warren R. Carithers on 2021/09/02.
//  Based on earlier versions created by Joe Geigel and Warren R. Carithers
//  Copyright 2021 Rochester Institute of Technology. All rights reserved.
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
#include "ShaderSetup.h"
#include "Buffers.h"

#include "Models.h"

using namespace std;

///
// PRIVATE GLOBALS
///

///
// Object information
///

// geometry data:  (x,y,z,w) coords
static Vertex datapoints[] = {
{ 0.25, -0.75, 0.0, 1.0 }, { 0.50, -0.75, 0.0, 1.0 }, { 0.25,  0.15, 0.0, 1.0 },
{ 0.50, -0.75, 0.0, 1.0 }, { 0.50,  0.15, 0.0, 1.0 }, { 0.25,  0.15, 0.0, 1.0 },
{ 0.25,  0.25, 0.0, 1.0 }, { 0.50,  0.25, 0.0, 1.0 }, { 0.25,  0.50, 0.0, 1.0 },
{ 0.50,  0.25, 0.0, 1.0 }, { 0.50,  0.50, 0.0, 1.0 }, { 0.25,  0.50, 0.0, 1.0 },
{-0.25, -0.75, 0.0, 1.0 }, { 0.00, -0.75, 0.0, 1.0 }, {-0.25,  0.75, 0.0, 1.0 },
{ 0.00, -0.75, 0.0, 1.0 }, { 0.00,  0.75, 0.0, 1.0 }, {-0.25,  0.75, 0.0, 1.0 },
{-0.75, -0.75, 0.0, 1.0 }, {-0.50, -0.75, 0.0, 1.0 }, {-0.75,  0.75, 0.0, 1.0 },
{-0.50, -0.75, 0.0, 1.0 }, {-0.50,  0.75, 0.0, 1.0 }, {-0.75,  0.75, 0.0, 1.0 },
{-0.50, -0.12, 0.0, 1.0 }, {-0.25, -0.12, 0.0, 1.0 }, {-0.50,  0.12, 0.0, 1.0 },
{-0.25, -0.12, 0.0, 1.0 }, {-0.25,  0.12, 0.0, 1.0 }, {-0.50,  0.12, 0.0, 1.0 }
};
// bytes in the datapoint array
static int dataSize = sizeof(datapoints);

// number of vertices in the model
static int nverts = sizeof(datapoints) / sizeof(Vertex);

static GLuint elements[] = {
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
     15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29
};
static int elemSize = sizeof(elements);

// color data:  (r,g,b,a) values
static Color colors1[] = {
{ 0.00, 1.00, 0.00, 1.0 }, { 0.00, 1.00, 0.00, 1.0 }, { 0.00, 0.28, 0.72, 1.0 },
{ 0.00, 1.00, 0.00, 1.0 }, { 0.00, 0.28, 0.72, 1.0 }, { 0.00, 0.28, 0.72, 1.0 },
{ 0.00, 0.20, 0.80, 1.0 }, { 0.00, 0.20, 0.80, 1.0 }, { 0.00, 0.00, 1.00, 1.0 },
{ 0.00, 0.20, 0.80, 1.0 }, { 0.00, 0.00, 1.00, 1.0 }, { 0.00, 0.00, 1.00, 1.0 },
{ 1.00, 0.00, 0.00, 1.0 }, { 1.00, 0.00, 0.00, 1.0 }, { 1.00, 0.00, 0.00, 1.0 },
{ 1.00, 0.00, 0.00, 1.0 }, { 1.00, 0.00, 0.00, 1.0 }, { 1.00, 0.00, 0.00, 1.0 },
{ 1.00, 1.00, 0.00, 1.0 }, { 1.00, 1.00, 0.00, 1.0 }, { 1.00, 1.00, 0.00, 1.0 },
{ 1.00, 1.00, 0.00, 1.0 }, { 1.00, 1.00, 0.00, 1.0 }, { 1.00, 1.00, 0.00, 1.0 },
{ 1.00, 1.00, 0.00, 1.0 }, { 1.00, 0.00, 0.00, 1.0 }, { 1.00, 1.00, 0.00, 1.0 },
{ 1.00, 0.00, 0.00, 1.0 }, { 1.00, 0.00, 0.00, 1.0 }, { 1.00, 1.00, 0.00, 1.0 }
};
static int colorSize1 = sizeof(colors1);

static Color colors2[] = {
{ 1.00, 0.00, 1.00, 1.0 }, { 1.00, 0.00, 1.00, 1.0 }, { 1.00, 0.72, 0.28, 1.0 },
{ 1.00, 0.00, 1.00, 1.0 }, { 1.00, 0.72, 0.28, 1.0 }, { 1.00, 0.72, 0.28, 1.0 },
{ 1.00, 0.80, 0.20, 1.0 }, { 1.00, 0.80, 0.20, 1.0 }, { 1.00, 1.00, 0.00, 1.0 },
{ 1.00, 0.80, 0.20, 1.0 }, { 1.00, 1.00, 0.00, 1.0 }, { 1.00, 1.00, 0.00, 1.0 },
{ 0.00, 1.00, 1.00, 1.0 }, { 0.00, 1.00, 1.00, 1.0 }, { 0.00, 1.00, 0.00, 1.0 },
{ 0.00, 1.00, 1.00, 1.0 }, { 0.00, 1.00, 1.00, 1.0 }, { 0.00, 1.00, 1.00, 1.0 },
{ 0.00, 0.00, 1.00, 1.0 }, { 0.00, 0.00, 1.00, 1.0 }, { 0.00, 0.00, 1.00, 1.0 },
{ 0.00, 0.00, 1.00, 1.0 }, { 0.00, 0.00, 1.00, 1.0 }, { 0.00, 0.00, 1.00, 1.0 },
{ 0.00, 0.00, 1.00, 1.0 }, { 0.00, 1.00, 1.00, 1.0 }, { 0.00, 0.00, 1.00, 1.0 },
{ 0.00, 1.00, 1.00, 1.0 }, { 0.00, 1.00, 1.00, 1.0 }, { 0.00, 0.00, 1.00, 1.0 }
};
static int colorSize2 = sizeof(colors2);

///
// PUBLIC GLOBALS
///

///
// PUBLIC FUNCTIONS
///

///
// Draw a shape
//
// This is essentially a combination of the Canvas and draw*() code
// from the first "real" lab assignments.
///
static void drawShape( Vertex v[], int vs, Color c[], int cs,
                       GLuint e[], int es, int nv, BufferSet &B ) {

    // make sure the buffer is empty
    if( B.bufferInit ) {
        glDeleteBuffers( 1, &(B.vbuffer) );
        glDeleteBuffers( 1, &(B.ebuffer) );
        B.initBuffer();
    }

    // number of elements in the buffer
    B.numElements = nv;

    // create the vertex buffer
    B.vbuffer = B.makeBuffer( GL_ARRAY_BUFFER, NULL, vs + cs );
    glBufferSubData( GL_ARRAY_BUFFER,  0, vs, v );
    glBufferSubData( GL_ARRAY_BUFFER, vs, cs, c );
    B.vSize = vs;
    B.cSize = cs;

    // create the element buffer
    B.ebuffer = B.makeBuffer( GL_ELEMENT_ARRAY_BUFFER, e, es );
    B.eSize = es;

    // mark it as "created"
    B.bufferInit = true;

}

///
// Create an object
///
void createObject( int which, BufferSet &B ) {

    if( which == 1 ) {
        // create the first shape
        drawShape( datapoints, dataSize, colors1, colorSize1,
                   elements, elemSize, nverts, B );
    } else {
        // same points, but different colors
        drawShape( datapoints, dataSize, colors2, colorSize2,
                   elements, elemSize, nverts, B );
    }
}
