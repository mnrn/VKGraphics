#version 410

layout (location=0) in vec3 VertexPosition;
layout (location=1) in vec3 VertexNormal;

out vec3 LightIntensity;

uniform struct LightInfo {
    vec4 Position;  // カメラ座標系から見たライトの位置
    vec3 La;        // Ambient Light (環境光)の強さ
    vec3 Ld;        // Diffuse Light (拡散光)の強さ
    vec3 Ls;        // Specular Light (鏡面反射光)の強さ 
} Light;

uniform struct MaterialInfo {
    vec3 Ka;  // Ambient reflectivity (環境光の反射係数)
    vec3 Kd;  // Diffsue reflectivity (拡散光の反射係数)
    vec3 Ks;  // Specular reflectivity (鏡面反射光の反射係数)
    float Shininess;  // Specular shininess factor (鏡面反射の強さの係数)
} Material;

uniform mat4 ModelViewMatrix;
uniform mat3 NormalMatrix;
uniform mat4 MVP;

void GetCamSpace(out vec3 pos, out vec3 norm) {
    pos = (ModelViewMatrix * vec4(VertexPosition, 1.0)).xyz;
    norm = normalize(NormalMatrix * VertexNormal);
}

void GetEyeSpace(out vec4 pos, out vec3 norm) {
    pos = ModelViewMatrix * vec4(VertexPosition, 1.0);
    norm = normalize(NormalMatrix * VertexNormal);
}


subroutine vec3 ShadeModelType(vec3 pos, vec3 norm);
subroutine uniform ShadeModelType ShadeModel;

subroutine(ShadeModelType)
vec3 PhongModel(vec3 p, vec3 n) {
    // Phong Shading Equation
    vec3 amb = Light.La * Material.Ka;
    vec3 s = normalize(vec3(Light.Position.xyz - p));
    float sDotN = max(dot(s, n), 0.0);
    vec3 diff = Light.Ld * Material.Kd  * sDotN;
    vec3 spec = vec3(0.0);
    if (sDotN > 0.0) {
        vec3 v = normalize(-p.xyz);
        vec3 r = reflect(-s, n);
        spec = Light.Ls * Material.Ks * pow(max(dot(r, v), 0.0), Material.Shininess);
    }
    return amb + diff + spec;
}

subroutine(ShadeModelType)
vec3 DiffuseModel(vec3 p, vec3 n) {
    vec3 s = normalize(Light.Position.xyz - p);
    return Light.Ld * Material.Kd * max(dot(s, n), 0.0);
}

void main() {
    // カメラ座標空間の位置情報と法線情報を取得する。
    vec3 p, n;
    GetCamSpace(p, n);

    // 選択されたシェーディングモデルを計算する。
    LightIntensity = ShadeModel(p, n);

    // 位置情報をクリップ座標系に変換して渡します。
    gl_Position = MVP * vec4(VertexPosition, 1.0);
}
