#version 450

layout (location = 0) in vec3 WorldPos;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec3 Color;

layout (location = 0) out vec4 PositionData;
layout (location = 1) out vec4 NormalData;
layout (location = 2) out vec4 AlbedoData;

void main() {
    PositionData = vec4(WorldPos, 1.0);
    NormalData = vec4(Normal, 1.0);
    AlbedoData = vec4(Color, 1.0);
}
