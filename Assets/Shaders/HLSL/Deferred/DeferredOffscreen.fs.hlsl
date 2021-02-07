struct VSOutput {
    [[vk::location(0)]] float3 WorldPos : POSITION0;
    [[vk::location(1)]] float3 Color : COLOR0;
    [[vk::location(2)]] float3 Normal : NORMAL0;
};

struct FSOutput {
    float4 Position : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float4 Albedo : SV_TARGET2;
};

FSOutput main(VSOutput input) {
    FSOutput output = (FSOutput)0;
    
    output.Position = float4(input.WorldPos, 1.0);
    output.Normal = float4(input.Normal, 1.0);
    output.Albedo = float4(input.Color, 1.0);

    return output;
}
