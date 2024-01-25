//
// Vertex shader for the "Hello, OpenGL!" assignment.
//
// Unlike the other 2D assignments, we don't do the normalization
// transformation here.
//

#version 150

// incoming vertex attributes
in vec4 vPosition;
in vec4 vColor;

// outgoing color sent to the fragment shader
out vec4 rescolor;

void main()
{
    gl_Position = vPosition;
    rescolor = vColor;
}
