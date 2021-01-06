#version 410

in vec3 Position;
in vec3 Normal;

layout (location=0) out vec4 FragColor;

const float PI = 3.14159265358979323846264;
const float GAMMA = 2.2;

uniform struct LightInfo {
    vec4 Position;  // カメラ座標系におけるライトの位置
    vec3 L;         // ライトの強さ
} Light[3];

uniform struct MaterialInfo {
    float Roughness; // 粗さ
    bool Metallic;   // 金属かどうか true=metal(導体,金属), false=dielectric(誘電体, 非金属)
    vec3 Color;      // 誘電体(Dielectric)のDiffuse色(Albedo)もしくは金属(Metal)のSpecular色
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
 * @brief The Smith geometric shadowing function (を簡単にした関数です。)
 */
float V_Smith(float NoV, float NoL, float roughness) {
    float GGXV = NoL * (NoV * (1.0 - roughness) + roughness);
    float GGXL = NoV * (NoL * (1.0 - roughness) + roughness);
    return 0.5 / (GGXV * GGXL);
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
    vec3 diff = vec3(0.0);
    if (!Material.Metallic) {
        diff = Material.Color;
    }
    // 金属(導体)ならSpecular色取得
    vec3 f0 = vec3(0.04);
    if (Material.Metallic) {
        f0 = Material.Color;
    }

    // ライトに関して。
    vec3 l = vec3(0.0);
    vec3 lightIntensity = Light[lightIdx].L;
    if (Light[lightIdx].Position.w == 0.0) {    // Directional Lightの場合
        l = normalize(Light[lightIdx].Position.xyz);
    } else {                                    // Positional Lightの場合 
        l = Light[lightIdx].Position.xyz - pos;
        float dist = length(l);
        l = normalize(l);
        lightIntensity /= (dist * dist);
    }

    vec3 v = normalize(-pos);   // 視線ベクトル
    vec3 h = normalize(v + l);  // ハーフベクトル(Bling-Phongモデルと同じ)
    float NoV = dot(n, v);      // abs(dot(n, v)) + 1e-5
    float NoL = dot(n, l);      // clamp(dot(n, l), 0.0, 1.0)
    float NoH = dot(n, h);      // clamp(dot(n, h), 0.0, 1.0)
    float LoH = dot(l, h);      // clamp(dot(l, h), 0.0, 1.0)

    // ラフネスをパラメタ化します。
    float roughness = Material.Roughness * Material.Roughness;

    // Specular BRDF
    float D = D_GGX(NoH, roughness);
    vec3 F = F_Schlick(LoH, f0);
    float V = V_Smith(NoV, NoL, roughness);
    vec3 spec = D * V * F;

    return (diff + PI * spec) * lightIntensity * NoL;
}

void main() {
    vec3 color = vec3(0.0);
    vec3 n = normalize(Normal);

    for (int i =0; i < 3; i++) {
        color += MicroFacetModel(i, Position, n);
    }

    FragColor = vec4(GammaCorrection(color), 1.0);
}
