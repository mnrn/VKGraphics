struct VSInput {
    [[vk::location(0)]] float3 Pos : POSITION0;
    [[vk::location(1)]] float3 Color : COLOR0;
    [[vk::location(2)]] float3 Normal : NORMAL0;
};

struct UniformBufferObject {
    float4x4 Model;
    float4x4 View;
    float4x4 Proj;
};

cbuffer ubo : register(b0) { UniformBufferObject ubo; }

struct VSOutput {
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float3 WorldPos : POSITION0;
    [[vk::location(1)]] float3 Color : COLOR0;
    [[vk::location(2)]] float3 Normal : NORMAL;
};

VSOutput main(VSInput input, uint instanceId : SV_INSTANCEID) {
    VSOutput output = (VSOutput)0;
    
    float4 localPos = float4(input.Pos, 1.0);
    output.Pos = mul(ubo.Proj, mul(ubo.View, mul(ubo.Model, localPos)));
    
    output.WorldPos = mul(ubo.Model, localPos);
    output.Color = input.Color;
    output.Normal = mul((float3x3)ubo.Model, input.Normal);

    return output;
}
