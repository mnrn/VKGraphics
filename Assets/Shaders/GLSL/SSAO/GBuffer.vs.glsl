#version 450

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec3 VertexColor;
layout (location = 3) in vec2 VertexUV;

layout (binding = 0) uniform UniformBufferObject {
    mat4 View;
    mat4 Proj;
} ubo;

layout (location = 0) out vec3 Position;
layout (location = 1) out vec3 Normal;
layout (location = 2) out vec3 Color;
layout (location = 3) out vec2 UV;

layout (push_constant) uniform PushConstants {
    mat4 Model;
} pushConsts;

void main () {
    Position = vec3(ubo.View * pushConsts.Model * vec4(VertexPosition, 1.0));

    mat3 normalMatrix = transpose(inverse(mat3(ubo.View * pushConsts.Model)));
    Normal = normalMatrix * VertexNormal;

    Color = VertexColor;
    UV = VertexUV;

    gl_Position = ubo.Proj * vec4(Position, 1.0);
}
