Texture2D FontTexture : register(t0);
SamplerState FontSampler : register(s0);

struct VSOutput {
    [[vk::location(0)]] float2 UV : TEXCOORD0;
    [[vk::location(1)]] float4 Color : COLOR0;
};

float4 main(VSOutput input) : SV_TARGET {
    return input.Color * FontTexture.Sample(FontSampler, input.UV);
}
