#version 450

layout (location=0) in vec3 VertexPosition;
layout (location=1) in vec3 VertexNormal;

layout (location=0) out vec3 Position;
layout (location=1) out vec3 Normal;

layout (binding=0) uniform UniformBufferObject {
    mat4 ViewProj;
} ubo;

layout (push_constant) uniform PushConstants {
    mat4 Model;
} pushConsts;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    Position = vec3(pushConsts.Model * vec4(VertexPosition, 1.0));
    Normal = mat3(pushConsts.Model) * VertexNormal;
    gl_Position = ubo.ViewProj * vec4(Position, 1.0);
}
