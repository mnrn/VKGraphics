#version 450

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec3 Color;
layout (location = 3) in vec2 UV;

layout (location = 0) out vec4 PositionData;
layout (location = 1) out vec4 NormalData;
layout (location = 2) out vec4 AlbedoData;

layout (binding = 1) uniform sampler2D Tex1;
layout (binding = 2) uniform sampler2D Tex2;

layout (push_constant) uniform PushConstants {
    mat4 Dummy;
    int Tex;
} pushConsts;

void main() {
    PositionData = vec4(Position, 1.0);
    NormalData = vec4(normalize(Normal), 1.0);
    switch (pushConsts.Tex) {
        case 1:
            AlbedoData = texture(Tex1, UV);
            break;
        case 2:
            AlbedoData = texture(Tex2, UV);
            break;
        default:
            AlbedoData = vec4(Color, 1.0);
            break;
    } 
}
