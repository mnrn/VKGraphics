static const float PI = 3.14159265358979323846264;
static const float GAMMA = 2.2;
static const int LIGHTS_MAX = 8;

struct VSOutput {
    [[vk::location(0)]] float3 WorldPos : POSITION0;
    [[vk::location(1)]] float3 Normal : NORMAL0;
};

struct LightInfo {
    float4 Position;
    float Intensity;
};

struct UniformBufferObjectShared {
    float3 CamPos;
    LightInfo Lights[LIGHTS_MAX];
    int LightsNum;
};

cbuffer uboParams : register(b1) {
    UniformBufferObjectShared uboParams;
}

struct PushConstants {
    [[vk::offset(12)]] float Roughness;
    [[vk::offset(16)]] float Metallic;
    [[vk::offset(20)]] float Reflectance;
    [[vk::offset(24)]] float R;
    [[vk::offset(28)]] float G;
    [[vk::offset(32)]] float B;
};

[[vk::push_constant]] PushConstants material;

float3 MaterialBaseColor() {
    return float3(material.R, material.G, material.B);
}

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
float3 F_Schlick(float u, float3 f0) {
    return f0 + (float3(1.0, 1.0, 1.0) - f0) * pow(1.0 - u, 5.0);
}

float3 GammaCorrection(float3 color) {
    return pow(color, float3(1.0 / GAMMA, 1.0 / GAMMA, 1.0 / GAMMA));
}

float3 MicroFacetModel(int lightIdx, float3 pos, float3 n) {
    // 誘電体(非金属)ならDiffuse色(Albedo)取得
    float3 diff = (1.0 - material.Metallic) * MaterialBaseColor();

    // 金属(導体)ならSpecular色取得
    float3 f0 = 0.16 * material.Reflectance * material.Reflectance * (1.0 - material.Metallic) + MaterialBaseColor() * material.Metallic;

    // ライトに関して。
    float3 l = float3(0.0, 0.0, 0.0);
    float lightIntensity = uboParams.Lights[lightIdx].Intensity;
    if (uboParams.Lights[lightIdx].Position.w == 0.0) {    // Directional Lightの場合
        l = normalize(uboParams.Lights[lightIdx].Position.xyz);
    } else {                                    // Positional Lightの場合 
        l = uboParams.Lights[lightIdx].Position.xyz - pos;
        float dist = length(l);
        l = normalize(l);
        lightIntensity /= (dist * dist);
    }

    float3 v = normalize(uboParams.CamPos - pos);   // 視線ベクトル
    float3 h = normalize(v + l);              // ハーフベクトル(Bling-Phongモデルと同じ)
    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp(dot(n, l), 0.0, 1.0);
    float NoH = clamp(dot(n, h), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);

    // ラフネスをパラメタ化します。
    float roughness = material.Roughness * material.Roughness;

    // Specular BRDF
    float D = D_GGX(NoH, roughness);
    float3 F = F_Schlick(LoH, f0);
    float V = V_SmithGGX(NoV, NoL, roughness);
    float3 spec = (D * V) * F;

    return (diff + PI * spec) * lightIntensity * NoL;
}

float4 main(VSOutput input) : SV_TARGET {
    float3 color = float3(0.0, 0.0, 0.0);
    float3 n = normalize(input.Normal);

    for (int i = 0; i < uboParams.LightsNum; i++) {
        color += MicroFacetModel(i, input.WorldPos, n);
    }

    return float4(GammaCorrection(color), 1.0);
}
