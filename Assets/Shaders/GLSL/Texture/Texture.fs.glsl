#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 1) uniform sampler2D Tex1;

layout (location = 0) in vec2 TexCoord;
layout (location = 1) in float LodBias;

layout (location = 0) out vec4 FragColor;

void main() {
    FragColor = vec4(texture(Tex1, TexCoord, LodBias).rgb, 1.0);
}
