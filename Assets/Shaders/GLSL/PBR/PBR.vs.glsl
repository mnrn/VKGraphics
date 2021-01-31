#version 450

layout (location=0) in vec3 VertexPosition;

layout (location=0) out vec3 Position;

layout (binding=0) uniform UniformBufferObject {
    mat4 Model;
    mat4 View;
    mat4 Proj;
} UBO;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    Position = vec3(UBO.Model * vec4(VertexPosition, 1.0));
    gl_Position = UBO.Proj * UBO.View * vec4(Position, 1.0);
}
