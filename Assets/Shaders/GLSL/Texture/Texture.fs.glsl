#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 1) uniform sampler2D Tex1;

layout (location = 0) in vec3 Color;
layout (location = 1) in vec2 TexCoord;

layout (location = 0) out vec4 FragColor;

void main() {
    FragColor = vec4(texture(Tex1, TexCoord).rgb * Color, 1.0);
}
