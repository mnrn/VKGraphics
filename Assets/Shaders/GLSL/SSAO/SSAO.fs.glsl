#version 450

layout (constant_id = 0) const int KERNEL_SIZE = 64;

layout (binding = 0) uniform sampler2D PositionDepthTex;
layout (binding = 1) uniform sampler2D NormalTex;
layout (binding = 2) uniform sampler2D RandRotTex;

layout (binding = 3) uniform UniformBufferObject {
    vec4 Samples[KERNEL_SIZE];
    mat4 Proj;
    float Radius;
    float Bias;
} ubo;

layout (location = 0) in vec2 UV;

layout (location = 0) out float FragColor;

void main() {
    vec3 pos = texture(PositionDepthTex, UV).rgb;
    vec3 norm = normalize(texture(NormalTex, UV).rgb);

    ivec2 texDim = textureSize(PositionDepthTex, 0);
    ivec2 noiseDim = textureSize(RandRotTex, 0);
    vec2 noiseUV = vec2(float(texDim.x) / float(noiseDim.x), float(texDim.y) / float(noiseDim.y)) * UV;
    vec3 randDir = normalize(texture(RandRotTex, noiseUV).xyz);

    // 接座標空間->カメラ座標空間変換行列を生成します。
    vec3 tang = normalize(randDir - norm * dot(randDir, norm));
    vec3 bitang = cross(norm, tang);
    mat3 TBN = mat3(tang, bitang, norm);

    // サンプリングを行い、AO(環境遮蔽)の係数値を計算します。
    float occ = 0.0;
    for (int i = 0; i < KERNEL_SIZE; i++) {
        vec3 samplePos = pos + ubo.Radius * (TBN * ubo.Samples[i].xyz);

        // カメラ座標->クリップ座標->正規化デバイス座標->テクスチャ座標
        vec4 p = ubo.Proj * vec4(samplePos, 1.0);
        p *= 1.0 / p.w;
        p.xyz = p.xyz * 0.5 + 0.5;

        // サンプル点と比較し、遮蔽されるようであれば環境遮蔽係数に加算します。
        float surfZ = texture(PositionDepthTex, p.xy).z;
        float range = smoothstep(0.0, 1.0, ubo.Radius / abs(pos.z - surfZ));
        occ += (surfZ >= samplePos.z + ubo.Bias ? 1.0 : 0.0) * range;
    }
    occ = 1.0 - (occ / float(KERNEL_SIZE));
    FragColor = occ;
}
