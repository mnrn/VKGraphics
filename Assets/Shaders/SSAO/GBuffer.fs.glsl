#version 410

const float kGamma = 2.2;

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;

layout (location=0) out vec3 PositionData;
layout (location=1) out vec3 NormalData;
layout (location=2) out vec3 ColorData;

uniform sampler2D DiffTex;

uniform struct MaterialInfo {
    vec3 Kd;        // Diffsue reflectivity (拡散光の反射係数)
    bool UseTex;    // テクスチャを使うかどうか
} Material;

void main() {
    // 位置情報、法線情報、色情報をGBufferに詰め込みます。
    PositionData = Position;
    NormalData = normalize(Normal);
    if (Material.UseTex) {
        ColorData = pow(texture(DiffTex, TexCoord.xy).xyz, vec3(kGamma));
    } else {
        ColorData = Material.Kd;
    }
}
