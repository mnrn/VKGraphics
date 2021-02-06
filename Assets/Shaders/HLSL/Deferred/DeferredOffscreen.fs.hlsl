struct VSOutput {
    [[vk::location(0)]] float3 WorldPos : POSITION0;
    [[vk::location(1)]] float2 UV : TEXCOORD0;
    [[vk::location(2)]] float3 Color : COLOR0;
    [[vk::location(3)]] float3 Normal : NORMAL0;
    [[vk::location(4)]] float3 Tangent : TANGENT0;
};

struct FSOutput {
    float4 Position : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float4 Albedo : SV_TARGET2;
};

Texture2D ColorTexture : register(t1);
SamplerState ColorSampler : register(s1);
Texture2D NormalMapTexture : register(t2);
SamplerState NormalMapSampler : register(s2);

FSOutput main(VSOutput input) {
    FSOutput output = (FSOutput)0;
    
    output.Position = float4(input.WorldPos, 1.0);

    // 接空間上の法線を計算します。
    float3 N = normalize(input.Normal);
    float3 T = normalize(input.Tangent);
    float3 B = cross(N, T);
    float3x3 TBN = float3x3(T, B, N);
    output.Normal = float4(mul(normalize(NormalMapTexture.Sample(NormalMapSampler, input.UV).xyz * 2.0 - float3(1.0, 1.0, 1.0)), TBN), 1.0);

    output.Albedo = ColorTexture.Sample(ColorSampler, input.UV);

    return output;
}
