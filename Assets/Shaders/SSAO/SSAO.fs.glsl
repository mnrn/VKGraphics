#version 410

const int kKernelSize = 64;
const vec2 kRandScale = vec2(1280.0 / 4.0, 720.0 / 4.0);

in vec2 TexCoord;

layout (location=0) out float FragColor;

uniform sampler2D PositionTex;
uniform sampler2D NormalTex;
uniform sampler2D RandRotTex;

uniform mat4 ProjectionMatrix;
uniform vec3 SampleKernel[kKernelSize];
uniform float Radius = 0.55;

void main() {
    // ランダムに接座標空間->カメラ座標空間変換行列を生成します。
    vec3 randDir = normalize(texture(RandRotTex, TexCoord.xy * kRandScale).xyz);
    vec3 n = normalize(texture(NormalTex, TexCoord.xy).xyz);
    vec3 bitang = cross(n, randDir);
    if (length(bitang) < 0.0001) {  // nとrandDirが平行であれば、nはx-y平面に存在します。
        bitang = cross(n, vec3(0, 0, 1));
    }
    bitang = normalize(bitang);
    vec3 tang = cross(bitang, n);
    mat3 toCamSpace = mat3(tang, bitang, n);

    // サンプリングを行い、AO(環境遮蔽)の係数値を計算します。
    float occ = 0.0;
    vec3 camPos = texture(PositionTex, TexCoord).xyz;
    for (int i = 0; i < kKernelSize; i++) {
        vec3 samplePos = camPos + Radius * (toCamSpace * SampleKernel[i]);

        // カメラ座標->クリッピング座標->正規化デバイス座標->テクスチャ座標
        vec4 p = ProjectionMatrix * vec4(samplePos, 1.0);
        p *= 1.0 / p.w;
        p.xyz = p.xyz * 0.5 + 0.5;

        // サンプル点と比較し、遮蔽されるようであれば環境遮蔽係数に加算します。
        float surfZ = texture(PositionTex, p.xy).z;
        float distZ = surfZ - camPos.z;
        if (distZ >= 0.0 && distZ <= Radius && surfZ > samplePos.z) {
            occ += 1.0;
        }
    }
    // normalized
    occ = occ / kKernelSize;
    FragColor = 1.0 - occ;
}
