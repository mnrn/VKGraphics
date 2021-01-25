#version 450

layout (location = 0) in vec2 VertexPosition;
layout (location = 1) in vec2 VertexUV;
layout (location = 2) in vec4 VertexColor;

layout (push_constant) uniform PushConstants {
    vec2 Scale;
    vec2 Translate;
} pushConstants;

layout (location = 0) out vec2 UV;
layout (location = 1) out vec4 Color;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    UV = VertexUV;
    Color = VertexColor;
    gl_Position = vec4(VertexPosition * pushConstants.Scale + pushConstants.Translate, 0.0, 1.0);
}
