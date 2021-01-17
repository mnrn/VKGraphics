float4 main([[vk::location(0)]] float3 color : COLOR0) : SV_TARGET
{
    return float4(color, 1.0);
}
