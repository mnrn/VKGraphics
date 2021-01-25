#version 450

layout (binding = 0) uniform sampler2D FontSampler;

layout (location = 0) in vec2 UV;
layout (location = 1) in vec4 Color;

layout (location = 0) out vec4 FragColor;

void main() {
    FragColor = Color * texture(FontSampler, UV);
}
