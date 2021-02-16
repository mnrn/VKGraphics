#version 450

layout (location = 0) in vec2 UV;

layout (location = 0) out float FragColor;

layout (binding = 0) uniform sampler2D AOTex;

void main () {
    vec2 texelSize = 1.0 / vec2(textureSize(AOTex, 0));
    float acc = 0.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            acc += texture(AOTex, UV + offset).r;
        }
    }
    FragColor = acc / 9.0;
}
