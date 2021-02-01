#version 450

const float PI = 3.14159265358979323846264;
const float GAMMA = 2.2;
const int LIGHTS_MAX = 8;

layout (location=0) in vec3 Position;
layout (location=1) in vec3 Normal;

layout (location=0) out vec4 FragColor;

struct LightInfo {
    vec4 Position;
    float Intensity;
};

layout (binding=1) uniform UniformBufferObjectShared {
    vec3 CamPos;
    LightInfo Lights[LIGHTS_MAX];
    int LightsNum;
} UBOParams;

layout (push_constant) uniform PushConstants {
    layout (offset=12) float Roughness;
    layout (offset=16) float Metallic;
    layout (offset=20) float Reflectance;
    layout (offset=24) float R;
    layout (offset=28) float G;
    layout (offset=32) float B;
} Material;

/**
 * @brief The GGX distribution (GGX分布関数)
 */
float D_GGX(float NoH, float roughness) {
    float a2 = roughness * roughness;
    float f = (NoH * a2 - NoH) * NoH + 1.0;
    return a2 / (PI * f * f);
}

/**
 * @brief The Smith geometric shadowing function
 */
float V_SmithGGX(float NoV, float NoL, float roughness) {
    float a2 = roughness * roughness;
    float GGXV = NoL * sqrt(NoV * (-NoV * a2 + NoV) + a2);
    float GGXL = NoV * sqrt(NoL * (-NoL * a2 + NoL) + a2);
    return 0.5 / (GGXV + GGXL);
}

/**
 * @brief The Schlick approximation for the Fresnel term(フレネル項のSchlick近似)
 */
vec3 F_Schlick(float u, vec3 f0) {
    return f0 + (vec3(1.0) - f0) * pow(1.0 - u, 5.0);
}

vec3 GammaCorrection(vec3 color) {
    return pow(color, vec3(1.0 / GAMMA));
}

vec3 MicroFacetModel(int lightIdx, vec3 pos, vec3 n) {
    // 誘電体(非金属)ならDiffuse色(Albedo)取得
    vec3 diff = (1.0 - Material.Metallic) * vec3(Material.R, Material.G, Material.B);

    // 金属(導体)ならSpecular色取得
    vec3 f0 = 0.16 * Material.Reflectance * Material.Reflectance * (1.0 - Material.Metallic) + vec3(Material.R, Material.G, Material.B) * Material.Metallic;

    // ライトに関して。
    vec3 l = vec3(0.0);
    float lightIntensity = UBOParams.Lights[lightIdx].Intensity;
    if (UBOParams.Lights[lightIdx].Position.w == 0.0) {    // Directional Lightの場合
        l = normalize(UBOParams.Lights[lightIdx].Position.xyz);
    } else {                                    // Positional Lightの場合 
        l = UBOParams.Lights[lightIdx].Position.xyz - pos;
        float dist = length(l);
        l = normalize(l);
        lightIntensity /= (dist * dist);
    }

    vec3 v = normalize(UBOParams.CamPos - pos);   // 視線ベクトル
    vec3 h = normalize(v + l);              // ハーフベクトル(Bling-Phongモデルと同じ)
    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp(dot(n, l), 0.0, 1.0);
    float NoH = clamp(dot(n, h), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);

    // ラフネスをパラメタ化します。
    float roughness = Material.Roughness * Material.Roughness;

    // Specular BRDF
    float D = D_GGX(NoH, roughness);
    vec3 F = F_Schlick(LoH, f0);
    float V = V_SmithGGX(NoV, NoL, roughness);
    vec3 spec = (D * V) * F;

    return (diff + PI * spec) * lightIntensity * NoL;
}

void main() {
    vec3 color = vec3(0.0);
    vec3 n = normalize(Normal);

    for (int i = 0; i < UBOParams.LightsNum; i++) {
        color += MicroFacetModel(i, Position, n);
    }

    FragColor = vec4(GammaCorrection(color), 1.0);
}
