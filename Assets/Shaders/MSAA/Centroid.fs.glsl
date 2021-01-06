#version 410

centroid in vec2 TexCoord;

layout (location=0) out vec4 FragColor;

void main() {
    vec3 color = vec3(0.0);
    if (TexCoord.s > 1.0) {
        color = vec3(1.0, 1.0, 0.0);
    }
    FragColor = vec4(color, 1.0);
}
