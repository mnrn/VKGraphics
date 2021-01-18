Texture2D ColorTexture : register(t1);
SamplerState ColorSampler : register(s1);

struct VSOutput {
    [[vk::location(0)]] float3 Color : COLOR0;
    [[vk::location(1)]] float2 UV : TEXCOORD0;
};

float4 main(VSOutput input) : SV_TARGET {
    float4 color = ColorTexture.SampleLevel(ColorSampler, input.UV, 0);
    return color * float4(input.Color, 1.0);
}
