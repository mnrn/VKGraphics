#version 450

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec3 VertexColor;

layout (binding = 0) uniform UniformBufferObject {
    mat4 View;
    mat4 Proj;
} ubo;

layout (location = 0) out vec3 WorldPos;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec3 Color;

layout (push_constant) uniform PushConstants {
    mat4 Model;
} pushConsts;

void main () {
    WorldPos = vec3(pushConsts.Model * vec4(VertexPosition, 1.0));
    Color = VertexColor;
    Normal = mat3(pushConsts.Model) * VertexNormal;
    
    gl_Position = ubo.Proj * ubo.View * vec4(WorldPos, 1.0);
}
