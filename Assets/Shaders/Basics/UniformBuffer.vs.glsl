#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout (location = 0) in vec2 VertexPosition;
layout (location = 1) in vec3 VertexColor;

layout (location = 0) out vec3 Color;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(VertexPosition, 0.0, 1.0);
    Color = VertexColor;
}