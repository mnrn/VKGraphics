#version 410

layout (location=0) in vec3 VertexPosition;
layout (location=1) in vec3 VertexNormal;
out vec3 LightIntensity;

uniform vec4 LightPosition;  // ライトの位置
uniform vec3 Kd;             // Diffuse(拡散光)反射係数
uniform vec3 Ld;             // Diffuse(拡散光)の強さ

uniform mat4 ModelViewMatrix;
uniform mat3 NormalMatrix;
uniform mat4 MVP;

void main() {
    // 位置情報と法線情報を Eye coordinates (カメラ座標系)に変換します。
    vec4 surf = ModelViewMatrix * vec4(VertexPosition, 1.0);
    vec3 n = normalize(NormalMatrix * VertexNormal);
    vec3 s = normalize(vec3(LightPosition - surf));

    // Diffuse Shading Equation
    LightIntensity = Ld * Kd * max(dot(s, n), 0.0);

    // 位置情報をクリップ座標系に変換して渡します。
    gl_Position = MVP * vec4(VertexPosition, 1.0);
}
