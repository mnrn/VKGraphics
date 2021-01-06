#version 410

layout (location=0) in vec4 VertexTexCoord;

out vec2 TexCoord;

uniform mat4 ProjMatrix;

void main()
{
    gl_Position = ProjMatrix * vec4(VertexTexCoord.xy, 0.0, 1.0);
    TexCoord = VertexTexCoord.zw;
}
