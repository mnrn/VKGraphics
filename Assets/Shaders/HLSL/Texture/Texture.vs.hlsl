struct VSInput {
    [[vk::location(0)]] float3 Pos : POSITION0;
    [[vk::location(1)]] float3 Color : COLOR0;
    [[vk::location(2)]] float2 UV : TEXCOORD0;
};

struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

cbuffer ubo : register(b0) {
    UniformBufferObject ubo;
}

struct VSOutput {
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float3 Color : COLOR0;
    [[vk::location(1)]] float2 UV : TEXCOORD0;
};

VSOutput main(VSInput input) {
    VSOutput output = (VSOutput)0;
    output.Pos = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.Pos, 1.0))));
    output.Color = input.Color;
    output.UV = input.UV;
    return output;
}
