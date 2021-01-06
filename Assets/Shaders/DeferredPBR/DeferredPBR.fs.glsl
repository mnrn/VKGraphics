#version 410

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;

layout (location=0) out vec4 FragColor;
layout (location=1) out vec4 PositionData;
layout (location=2) out vec4 NormalData;
layout (location=3) out vec4 ColorData;

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

uniform sampler2D PositionTex;
uniform sampler2D NormalTex;
uniform sampler2D ColorTex;
uniform int Pass;

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

vec3 MicroFacetModel(int lightIdx, vec3 pos, vec3 n, vec3 diff, vec3 f0, float rough) {
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
    float roughness = rough * rough;

    // Specular BRDF
    float D = D_GGX(NoH, roughness);
    vec3 F = F_Schlick(LoH, f0);
    float V = V_Smith(NoV, NoL, roughness);
    vec3 spec = D * V * F;

    return (diff + PI * spec) * lightIntensity * NoL;
}

void PackGBuffer() {
    // 位置情報、法線情報、色情報をGBufferに詰め込みます。
    PositionData = vec4(Position, Material.Roughness);
    NormalData = vec4(normalize(Normal), Material.Metallic);
    ColorData = vec4(Material.Color, 1.0);
}

void VisualizeGBuffer() {
    // テクスチャから情報を取り出します。
    vec4 pbuf = texture(PositionTex, TexCoord);
    vec3 pos = pbuf.xyz;
    float rough = pbuf.w;

    vec4 nbuf = texture(NormalTex, TexCoord);
    vec3 norm = nbuf.xyz;
    float metallic = nbuf.w;
    
    // 誘電体(非金属)ならDiffuse色(Albedo)取得
    vec3 diff = vec3(0.0);
    if (metallic < 0.99) {
        diff = (texture(ColorTex, TexCoord)).rgb;
    }
    // 金属(導体)ならSpecular色取得
    vec3 f0 = vec3(0.04);
    if (metallic > 0.99) {
        f0 = (texture(ColorTex, TexCoord)).rgb;
    }

    vec3 color = vec3(0.0);
    for (int i =0; i < 3; i++) {
        color += MicroFacetModel(i, pos, norm, diff, f0, rough);
    }

    FragColor = vec4(GammaCorrection(color), 1.0);
}

void main() {
    if (Pass == 1) {
        PackGBuffer();
    } else if (Pass == 2) {
        VisualizeGBuffer();
    }
}
