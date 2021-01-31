#version 450

const float PI = 3.14159265358979323846264;
const float Gamma = 2.2;
const int LightMax = 2;

layout (location=0) in vec3 Position;

layout (location=0) out vec4 FragColor;

layout (binding=0) uniform UniformBufferObject {
    mat4 Model;
    mat4 View;
    mat4 Proj;
} UBO;

void main() {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
