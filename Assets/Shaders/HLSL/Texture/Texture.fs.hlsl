Texture2D ColorTexture : register(t1);
SamplerState ColorSampler : register(s1);

struct VSOutput {
    [[vk::location(0)]] float2 UV : TEXCOORD0;
};

float4 main(VSOutput input) : SV_TARGET {
    return ColorTexture.SampleLevel(ColorSampler, input.UV, 0);
}
