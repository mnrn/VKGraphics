#version 450

layout (location = 0) in vec2 UV;

layout (binding = 1) uniform sampler2D PosTex;
layout (binding = 2) uniform sampler2D NormTex;
layout (binding = 3) uniform sampler2D AlbedoTex;

layout (location = 0) out vec4 FragColor;

struct Light {
    vec4 Position;
    vec3 Color;
    float Radius;
};

layout (binding = 4) uniform UniformBufferObject {
    Light Lights[8];
    vec4 ViewPos;
    int LightsNum;
    int DisplayRenderTarget;
} ubo;

vec3 BlinnPhongModel(vec3 pos, vec3 norm, vec4 albedo, int lightIdx) {
    // ライトのベクトルを計算します。
    vec3 L = ubo.Lights[lightIdx].Position.xyz - pos;
    float dist = length(L);
    L = normalize(L);

    // 視線のベクトルを計算します。
    vec3 V = normalize(ubo.ViewPos.xyz - pos);

    // 減衰します。
    float atten = ubo.Lights[lightIdx].Position.w == 0.0
        ? 1.0 
        : ubo.Lights[lightIdx].Radius / (pow(dist, 2.0) + 1.0);

    // ディフューズを計算します。
    vec3 N = normalize(norm);
    float NoL = clamp((dot(N, L)), 0.0, 1.0);
    vec3 diff = ubo.Lights[lightIdx].Color * albedo.rgb * NoL;

    // スペキュラを計算します。
    vec3 H = normalize(V + L);
    float NoH = clamp((dot(N, H)), 0.0, 1.0);
    vec3 spec = ubo.Lights[lightIdx].Color * pow(NoH, 16.0f);

    return (diff + spec) * atten;
}

void main() {
    // G-Bufferから値を取得します。
    vec3 pos = texture(PosTex, UV).rgb;
    vec3 norm = texture(NormTex, UV).rgb;
    vec4 albedo = texture(AlbedoTex, UV);

    // デバッグなどに使用します。
    vec3 fragColor = vec3(0.0);
    if (ubo.DisplayRenderTarget > 0) {
        switch (ubo.DisplayRenderTarget) {
            case 1: 
                fragColor = pos;
                break;
            case 2:
                fragColor = norm;
                break;
            case 3:
                fragColor = albedo.rgb;
                break;
        }
        FragColor = vec4(fragColor, 1.0);
        return;
    }
    
    for (int i = 0 ; i < ubo.LightsNum; i++) {
        fragColor += BlinnPhongModel(pos, norm, albedo, i);
    }
    FragColor = vec4(fragColor, 1.0);
}
