#version 410

const float kGamma = 2.2;

in vec2 TexCoord;

layout (location=0) out vec4 FragColor;

uniform sampler2D PositionTex;
uniform sampler2D NormalTex;
uniform sampler2D ColorTex;
uniform sampler2D AOTex;

uniform int Type = 0;
uniform float AO = 8.0;

uniform struct LightInfo {
    vec4 Position;  // カメラ座標系におけるライトの位置
    vec3 L;         // Diffuse Light (拡散光)およびSpecular Light (鏡面反射光)の強さ
    vec3 La;        // Ambient Light (環境光)の強さ
} Light;

vec3 GammaCorrection(vec3 color) {
    return pow(color, vec3(1.0 / kGamma));
}

vec3 AmbientDiffuseModel(vec3 pos, vec3 norm, vec3 diff, float ao) {
    vec3 amb = Light.La * diff * ao;
    vec3 dirToL = normalize(vec3(Light.Position) - pos);
    float NoL = max(dot(norm, dirToL), 0.0);
    if (Type == 1) {
        return vec3(ao);
    } else if (Type == 2) {
        return Light.L * diff * NoL;
    } else {
        return amb + Light.L * diff * NoL;
    }
}

void main() {
    // テクスチャから情報を取り出します。
    vec3 pos = texture(PositionTex, TexCoord).xyz;
    vec3 norm = texture(NormalTex, TexCoord).xyz;
    vec3 diff = texture(ColorTex, TexCoord).rgb;
    float ao = texture(AOTex, TexCoord).r;

    // AOのパラメータ化
    ao = pow(ao, AO);

    vec3 color = AmbientDiffuseModel(pos, norm, diff, ao);
    color = GammaCorrection(color);
    FragColor = vec4(color, 1.0);
}
