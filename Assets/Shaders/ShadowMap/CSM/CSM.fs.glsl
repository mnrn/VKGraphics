#version 410

const float kGamma = 2.2;
const int kCascadesMax = 8;

in vec3 Position;
in vec3 Normal;

layout(location=0) out vec4 FragColor;

uniform struct LightInfo {
    vec4 Position;  // カメラ座標系から見たライトの位置
    vec3 La;        // Ambient Light (環境光)の強さ
    vec3 Ld;        // Diffuse Light (拡散光)の強さ
    vec3 Ls;        // Specular Light (鏡面反射光)の強さ 
} Light;

uniform struct MaterialInfo {
    vec3 Ka;  // Ambient reflectivity (環境光の反射係数)
    vec3 Kd;  // Diffsue reflectivity (拡散光の反射係数)
    vec3 Ks;  // Specular reflectivity (鏡面反射光の反射係数)
    float Shininess;  // Specular shininess factor (鏡面反射の強さの係数)
} Material;

uniform sampler2DArrayShadow ShadowMaps;
uniform int CascadesNum;
uniform float CameraHomogeneousSplitPlanes[kCascadesMax];
uniform mat4 ShadowMatrices[kCascadesMax];

uniform bool IsPCF = true;
uniform bool IsShadowOnly = false;
uniform bool IsVisibleIndicator = false;

vec4 GammaCorrection(vec4 color) {
    return pow(color, vec4(1.0 / kGamma));
}

vec3 PhongDSModel(vec3 pos, vec3 n) {
    // Phong Shading Equation
    vec3 s = normalize(vec3(Light.Position.xyz - pos));
    float sDotN = max(dot(s, n), 0.0);
    vec3 diff = Light.Ld * Material.Kd  * sDotN;
    vec3 spec = vec3(0.0);
    if (sDotN > 0.0) {
        vec3 v = normalize(-pos.xyz);
        vec3 r = reflect(-s, n);
        spec = Light.Ls * Material.Ks * pow(max(dot(r, v), 0.0), Material.Shininess);
    }
    return diff + spec;
}

float ComputeShadow(int idx) {
    // ModelView空間からライトから見たクリップ空間へ変換します。
    vec4 clippedShadowCoord = ShadowMatrices[idx] * vec4(Position, 1.0);
    vec4 shadowCoord = clippedShadowCoord / clippedShadowCoord.w;

    // シャドウのバイアスを計算します。厳密には傾斜に沿って計算してください。
    float bias = 0.00001 / clippedShadowCoord.w;

    if (IsPCF) {
        float shadow = 0.0;
        vec2 texelSize = 1.0 / textureSize(ShadowMaps, 0).xy;
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                shadow += texture(ShadowMaps, vec4(shadowCoord.xy + vec2(x, y) * texelSize, float(idx), shadowCoord.z - bias));
            }
        }
        return shadow / 9.0;
    } else {
        return texture(ShadowMaps, vec4(shadowCoord.xy, float(idx), shadowCoord.z - bias));
    }
}

vec4 ShadowIndicator(int idx) {
    if (idx == 0) {
        return vec4(0.1, 0.0, 0.0, 0.0);
    } else if (idx == 1) {
        return vec4(0.0, 0.1, 0.0, 0.0);
    } else if (idx == 2) {
        return vec4(0.0, 0.0, 0.1, 0.0);
    } else if (idx == 3) {
        return vec4(0.1, 0.1, 0.0, 0.0);
    } else if (idx == 4) {
        return vec4(0.0, 0.1, 0.1, 0.0);
    } else if (idx == 5) {
        return vec4(0.1, 0.0, 0.1, 0.0);
    } else if (idx == 6) {
        return vec4(0.1, 0.1, 0.1, 0.0);
    } else {
        return vec4(0.0, 0.0, 0.0, 0.0);
    }
}

void ShadeWithShadow() {
    vec3 amb = Light.La * Material.Ka;
    vec3 diffSpec = PhongDSModel(Position, Normal);

    // 該当するシャドウマップを探します。
    int idx = 0;
    for (int i = 0; i < CascadesNum; i++) {
        if (gl_FragCoord.z <= CameraHomogeneousSplitPlanes[i]) {
            idx = i;
            break;
        }
    }
    // 影の算出を行います。
    float shadow = ComputeShadow(idx);

    // ピクセルが影の中にある場合、Ambient Light (環境光)のみ使用することになります。
    vec4 color = vec4(diffSpec * shadow + amb, 1.0);

    if (IsShadowOnly) {
        color = vec4(shadow, shadow, shadow, 1.0);
    }
    if (IsVisibleIndicator) {
        color += ShadowIndicator(idx); 
    }
    FragColor = GammaCorrection(color);
}

void main() {
    ShadeWithShadow();
}
