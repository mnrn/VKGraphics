#version 450

const float GAMMA = 2.2;

layout (location = 0) in vec2 UV;

layout (binding = 1) uniform sampler2D PosTex;
layout (binding = 2) uniform sampler2D NormTex;
layout (binding = 3) uniform sampler2D AlbedoTex;
layout (binding = 4) uniform sampler2D AOTex;
layout (binding = 5) uniform sampler2D AOBlurTex;

layout (location = 0) out vec4 FragColor;

struct Light {
    vec4 Position;
    vec3 La;
    vec3 Ld;
};

layout (binding = 0) uniform UniformBufferObject {
    Light Lights[8];
    int LightsNum;
    int DisplayRenderTarget;
    bool UseBlur;
    float AO;
} ubo;

vec3 AmbientDiffuseModel(vec3 pos, vec3 norm, vec3 albedo, float ao, int idx) {
    vec3 amb = ubo.Lights[idx].La * albedo * ao;
    vec3 L = normalize(vec3(ubo.Lights[idx].Position) - pos);
    float NoL = max(dot(norm, L), 0.0);

    switch (ubo.DisplayRenderTarget) {
        case 2:
            return ubo.Lights[idx].Ld * albedo * NoL;
        default:
            return amb + ubo.Lights[idx].Ld * albedo * NoL;
    }
}
void main() {
    // G-Bufferから値を取得します。
    vec3 pos = texture(PosTex, UV).xyz;
    vec3 norm = texture(NormTex, UV).xyz;
    vec3 albedo = texture(AlbedoTex, UV).rgb;
    float ao = ubo.UseBlur
        ? texture(AOBlurTex, UV).r 
        : texture(AOTex, UV).r;

    // aoのパラメータ化を行います。
    ao = pow(ao, ubo.AO);

    vec3 fragColor = vec3(0.0);
    for (int i = 0; i < ubo.LightsNum; i++) {
        fragColor += AmbientDiffuseModel(pos, norm, albedo, ao, i);
    }
    fragColor = pow(fragColor, vec3(1.0 / GAMMA));

    switch (ubo.DisplayRenderTarget) {
        case 1:
            fragColor = vec3(ao); 
            break;
        case 3:
            fragColor = pos;
            break;
        case 4:
            fragColor = norm;
            break;
        case 5:
            fragColor = albedo;
            break;
    }
    FragColor = vec4(fragColor, 1.0);
}
