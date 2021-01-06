#version 430

layout (location = 0) in vec4 VertexPosition;

uniform mat4 MVP;

void main(void) {
    gl_Position = MVP * VertexPosition;
}
