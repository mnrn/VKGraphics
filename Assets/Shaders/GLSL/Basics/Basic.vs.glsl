#version 450

layout (binding = 0) uniform UniformBufferObject {
    mat4 Model;
    mat4 View;
    mat4 Proj;
} ubo;

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexColor;

layout (location = 0) out vec3 Color;

void main() {
    gl_Position = ubo.Proj * ubo.View * ubo.Model * vec4(VertexPosition, 1.0);
    Color = VertexColor;
}
