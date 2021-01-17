struct VSInput {
    [[vk::location(0)]] float2 pos : POSITION0;
    [[vk::location(1)]] float3 color : COLOR0;
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
    float4 pos : SV_POSITION;
    [[vk::location(0)]] float3 color : COLOR0;
};

VSOutput main(VSInput i) {
    VSOutput o = (VSOutput)0;
    o.pos = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(i.pos, 0.0, 1.0))));
    o.color = i.color;
    return o;
}
