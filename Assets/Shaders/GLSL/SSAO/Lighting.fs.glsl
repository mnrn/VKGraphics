#version 450

layout (binding = 0) uniform sampler2D PositionTex;
layout (binding = 1) uniform sampler2D NormalTex;
layout (binding = 2) uniform sampler2D AlbedoTex;
layout (binding = 3) uniform sampler2D AOTex;
layout (binding = 4) uniform sampler2D AOBlurTex;

struct LightInfo {
    vec4 Position;
    vec3 Ld;
    vec3 La; 
};

layout (binding = 5) uniform UniformBufferObject {
    mat4 Dummy;
    LightInfo Light;
    int DisplayRenderTarget;
    bool UseBlur;
    float AO;
} ubo;

layout (location = 0) in vec2 UV;
layout (location = 0) out vec4 FragColor;

vec3 AmbientDiffuseModel(vec3 pos, vec3 norm, vec3 albedo, float ao) {
    if (ubo.DisplayRenderTarget == 1) {
        return vec3(ao);    
    }

    vec3 amb = ubo.Light.La * albedo * ao;
    vec3 L = normalize(vec3(ubo.Light.Position) - pos);
    float NoL = max(dot(norm, L), 0.0);
    return ubo.DisplayRenderTarget == 2
        ?  ubo.Light.Ld * albedo * NoL
        : amb + ubo.Light.Ld * albedo * NoL;
}

void main () {
    vec3 pos = texture(PositionTex, UV).rgb;
    vec3 norm = normalize(texture(NormalTex, UV).rgb);
    vec3 albedo = texture(AlbedoTex, UV).rgb;

    float ao = ubo.UseBlur
        ? texture(AOBlurTex, UV).r 
        : texture(AOTex, UV).r;

    // aoのパラメータ化
    ao = pow(ao, ubo.AO);

    FragColor = vec4(AmbientDiffuseModel(pos, norm, albedo, ao), 1.0);
}
