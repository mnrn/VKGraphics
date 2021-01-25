#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 MVP;
    float LodBias;
} ubo;

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec2 VertexTexCoord;

layout (location = 0) out vec2 TexCoord;
layout (location = 1) out float LodBias;

void main() {
    gl_Position = ubo.MVP * vec4(VertexPosition, 1.0);
    TexCoord = VertexTexCoord;
    LodBias = ubo.LodBias;
}
