
Texture2D PosTex : register (t1);
SamplerState PosSamp : register(s1);
Texture2D NormTex : register(t2);
SamplerState NormSamp : register(s2);
Texture2D AlbedoTex : register(t3);
SamplerState AlbedoSamp : register(s3);

struct Light {
    float4 Position;
    float3 Color;
    float Radius;
};

struct UniformBufferObject {
    Light Lights[8];
    float4 ViewPos;
    int LightsNum;
    int DisplayRenderTarget;
};

cbuffer ubo : register(b4) { 
    UniformBufferObject ubo;
}

float3 BlinnPhongModel(float3 pos, float3 norm, float4 albedo, int lightIdx) {
    // ライトのベクトルを計算します。
    float3 L = ubo.Lights[lightIdx].Position.xyz - pos;
    float dist = length(L);
    L = normalize(L);

    // 視線のベクトルを計算します。
    float3 V = normalize(ubo.ViewPos.xyz - pos);

    // 減衰します。
    float atten = ubo.Lights[lightIdx].Position.w == 0.0
        ? 1.0 
        : ubo.Lights[lightIdx].Radius / (pow(dist, 2.0) + 1.0);

    // ディフューズを計算します。
    float3 N = normalize(norm);
    float NoL = saturate(dot(N, L));
    float3 diff = ubo.Lights[lightIdx].Color * albedo.rgb * NoL;

    // スペキュラを計算します。
    float3 H = normalize(V + L);
    float NoH = saturate(dot(N, H));
    float3 spec = ubo.Lights[lightIdx].Color * pow(NoH, 16.0f);

    return (diff + spec) * atten;
}

float4 main([[vk::location(0)]] float2 uv : TEXCOORD0) : SV_TARGET {
    // G-Bufferから値を取得します。
    float3 pos = PosTex.Sample(PosSamp, uv).rgb;
    float3 norm = NormTex.Sample(NormSamp, uv).rgb;
    float4 albedo = AlbedoTex.Sample(AlbedoSamp, uv);

    // デバッグなどに使用します。
    float3 fragColor = float3(0.0);
    if (ubo.DisplayRenderTarget > 0) {
        switch (ubo.DisplayRenderTarget) {
            case 1: 
                fragColor = pos;
                break;
            case 2:
                fragColor = norm;
                break;
            case 3:
                fragColor = albedo.rgb;
                break;
        }
        return float4(fragColor, 1.0);
    }

    for (int i = 0 ; i < ubo.LightsNum; i++) {
        fragColor += BlinnPhongModel(pos, norm, albedo, i);
    }
    return float4(fragColor, 1.0);
}
