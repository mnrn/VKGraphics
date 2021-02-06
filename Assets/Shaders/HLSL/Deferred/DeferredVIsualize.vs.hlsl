struct VSOutput {
    float4 Pos : SV_POSITION;
    [[vk::location(0)]] float2 UV : TEXCOORD0;
};

VSOutput main(uint vertexId : SV_VERTEXID) {
    VSOutput output = (VSOutput)0;
    output.UV = float2((vertexId << 1) & 2, vertexId & 2);
    output.Pos = float4(output.UV * 2.0 - 1.0, 0.0, 1.0);
    return output;
}
