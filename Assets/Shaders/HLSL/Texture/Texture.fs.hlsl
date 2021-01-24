Texture2D ColorTexture : register(t1);
SamplerState ColorSampler : register(s1);

struct VSOutput {
    [[vk::location(0)]] float2 UV : TEXCOORD0;
    [[vk::location(1)]] float LodBias : TEXCOORD1;
};

float4 main(VSOutput input) : SV_TARGET {
    return ColorTexture.SampleLevel(ColorSampler, input.UV, input.LodBias);
}
