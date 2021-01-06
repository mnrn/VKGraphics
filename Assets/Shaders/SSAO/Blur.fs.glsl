#version 410

layout (location=0) out float FragColor;

uniform sampler2D AOTex;

void main() {
    ivec2 pix = ivec2(gl_FragCoord.xy);
    float acc = 0.0;

    acc += texelFetchOffset(AOTex, pix, 0, ivec2(-1, -1)).r;
    acc += texelFetchOffset(AOTex, pix, 0, ivec2(-1, -0)).r;
    acc += texelFetchOffset(AOTex, pix, 0, ivec2(-1, 1)).r;
    acc += texelFetchOffset(AOTex, pix, 0, ivec2(0, -1)).r;
    acc += texelFetchOffset(AOTex, pix, 0, ivec2(0, 0)).r;
    acc += texelFetchOffset(AOTex, pix, 0, ivec2(0, 1)).r;
    acc += texelFetchOffset(AOTex, pix, 0, ivec2(1, -1)).r;
    acc += texelFetchOffset(AOTex, pix, 0, ivec2(1, 0)).r;
    acc += texelFetchOffset(AOTex, pix, 0, ivec2(1, 1)).r;
    
    // normalized
    float ao = acc * (1.0 / 9.0);
    FragColor = ao;
}
