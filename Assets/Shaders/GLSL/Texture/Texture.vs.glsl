#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexColor;
layout (location = 2) in vec2 VertexTexCoord;

layout (location = 0) out vec3 Color;
layout (location = 1) out vec2 TexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(VertexPosition, 1.0);
    Color = VertexColor;
    TexCoord = VertexTexCoord;
}
