#version 450

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec3 Color;
layout (location = 3) in vec2 UV;

layout (location = 0) out vec4 PositionData;
layout (location = 1) out vec4 NormalData;
layout (location = 2) out vec4 AlbedoData;

layout (binding = 0) uniform UniformBufferObject {
    mat4 Model;
    mat4 View;
    mat4 Proj;
    float NearPlane;
    float FarPlane;
} ubo;

float LinearDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * ubo.NearPlane * ubo.FarPlane) / (ubo.FarPlane + ubo.NearPlane - z * (ubo.FarPlane - ubo.NearPlane));
}

void main() {
    PositionData = vec4(Position, LinearDepth(gl_FragCoord.z));
    NormalData = vec4(normalize(Normal), 1.0);
    AlbedoData = vec4(Color, 1.0);
}
