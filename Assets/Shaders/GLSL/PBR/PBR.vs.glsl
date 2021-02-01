#version 450

layout (location=0) in vec3 VertexPosition;
layout (location=1) in vec3 VertexNormal;

layout (location=0) out vec3 Position;
layout (location=1) out vec3 Normal;

layout (binding=0) uniform UniformBufferObject {
    mat4 Model;
    mat4 View;
    mat4 Proj;
} ubo;

layout (push_constant) uniform PushConstants {
    vec3 ObjPos;
} pushConsts;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    Position = vec3(ubo.Model * vec4(VertexPosition, 1.0)) + pushConsts.ObjPos;
    Normal = mat3(ubo.Model) * VertexNormal;
    gl_Position = ubo.Proj * ubo.View * vec4(Position, 1.0);
}
