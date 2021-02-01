struct VSInput {
    [[vk::location(0)]] float3 Position : POSITION0;
    [[vk::location(1)]] float3 Normal : NORMAL0;
};

struct UniformBufferObject {
    float4x4 Model;
    float4x4 View;
    float4x4 Proj;
};

cbuffer ubo :register(b0) {
    UniformBufferObject ubo;
}

struct VSOutput {
    float4 Position : SV_POSITION;
    [[vk::location(0)]] float3 WorldPos : POSITION0;
    [[vk::location(1)]] float3 Normal : NORMAL0;
};

struct PushConsts {
    float3 ObjPos;
};
[[vk::push_constant]] PushConsts pushConsts;

VSOutput main(VSInput input) {
    VSOutput output = (VSOutput)0;
    output.WorldPos = mul(ubo.Model, float4(input.Position, 1.0)).xyz + pushConsts.ObjPos;
    output.Normal = mul((float3x3)ubo.Model, input.Normal);
    output.Position = mul(ubo.Proj, mul(ubo.View, float4(output.WorldPos, 1.0)));
    return output;
}
