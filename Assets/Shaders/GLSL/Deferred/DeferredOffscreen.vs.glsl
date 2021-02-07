#version 450

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexColor;
layout (location = 2) in vec3 VertexNormal;

layout (binding = 0) uniform UniformBufferObject {
    mat4 Model;
    mat4 View;
    mat4 Proj;
} ubo;

layout (location = 0) out vec3 WorldPos;
layout (location = 1) out vec3 Color;
layout (location = 2) out vec3 Normal;

void main () {
    gl_Position = ubo.Proj * ubo.View * ubo.Model * vec4(VertexPosition, 1.0);

    WorldPos = vec3(ubo.Model * vec4(VertexPosition, 1.0));
    Color = VertexColor;
    Normal = mat3(ubo.Model) * VertexNormal;
}
