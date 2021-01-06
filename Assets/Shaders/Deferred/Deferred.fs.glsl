#version 410

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;

layout (location=0) out vec4 FragColor;
layout (location=1) out vec3 PositionData;
layout (location=2) out vec3 NormalData;
layout (location=3) out vec3 ColorData;

uniform struct LightInfo {
    vec4 Position;  // カメラ座標系から見たライトの位置
    vec3 La;        // Ambient Light (環境光)の強さ
    vec3 Ld;        // Diffuse Light (拡散光)の強さ
    vec3 Ls;        // Specular Light (鏡面反射光)の強さ 
} Light;

uniform struct MaterialInfo {
    vec3 Kd;            // Diffuse reflecivity
} Material;

uniform sampler2D PositionTex;
uniform sampler2D NormalTex;
uniform sampler2D ColorTex;

uniform int Pass;

vec3 DiffuseModel(vec3 pos, vec3 norm, vec3 diff) {
    vec3 s = normalize(vec3(Light.Position) - pos);
    float sDotN = max(dot(s, norm), 0.0);
    return Light.Ld * diff * sDotN;
}

void PackGBuffer() {
    // 位置情報、法線情報、色情報をGBufferに詰め込みます。
    PositionData = Position;
    NormalData = normalize(Normal);
    ColorData = Material.Kd;
}

void VisualizeGBuffer() {
    // テクスチャから情報を取り出します。
    vec3 pos = vec3(texture(PositionTex, TexCoord));
    vec3 norm = vec3(texture(NormalTex, TexCoord));
    vec3 diff = vec3(texture(ColorTex, TexCoord));

    FragColor = vec4(DiffuseModel(pos, norm, diff), 1.0);
}

void main() {
    if (Pass == 1) {
        PackGBuffer();
    } else if (Pass == 2) {
        VisualizeGBuffer();
    }
}
