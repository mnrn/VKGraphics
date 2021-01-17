struct VSInput {
    [[vk::location(0)]] float2 pos : POSITION0;
    [[vk::location(1)]] float3 color : COLOR0;
};

struct VSOutput {
    float4 pos : SV_POSITION;
    [[vk::location(0)]] float3 color : COLOR0;
};

VSOutput main(VSInput i)
{
    VSOutput o = (VSOutput)0;
    o.pos = float4(i.pos, 0.0, 1.0);
    o.color = i.color;
    return o;
}
