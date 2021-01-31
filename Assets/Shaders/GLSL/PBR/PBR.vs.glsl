#version 450

layout (location=0) in vec3 VertexPosition;
layout (location=1) in vec3 VertexNormal;

layout (location=0) out vec3 Position;
layout (location=1) out vec3 Normal;

layout (binding=0) uniform UniformBufferObject {
    mat4 Model;
    mat4 View;
    mat4 Proj;
    vec3 CamPos;
} UBO;

layout (push_constant) uniform PushConstants {
    vec3 ObjPos;
} PushConsts;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    Position = vec3(UBO.Model * vec4(VertexPosition, 1.0)) + PushConsts.ObjPos;
    Normal = mat3(UBO.Model) * VertexNormal;
    gl_Position = UBO.Proj * UBO.View * vec4(Position, 1.0);
}
