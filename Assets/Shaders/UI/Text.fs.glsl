#version 410

in vec2 TexCoord;

layout (location=0) out vec4 FragColor;

uniform sampler2D Tex;
uniform vec4 Color = vec4(0.0, 0.0, 0.0, 1.0);

void main()
{
    FragColor = vec4(1.0, 1.0, 1.0, texture(Tex, TexCoord).r) * Color;
}
