#version 410

in vec3 Position;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D Tex1;

struct LightInfo {
    vec4 Position;   // カメラ座標系から見たライトの位置
    vec3 L;          // Diffuse, Specular intensity
    vec3 La;         // Ambient intensity
};
uniform LightInfo Light;

struct MaterialInfo {
    vec3 Ks;            // Specular reflecivity
    float Shininess;    // Specular shiness factor
};
uniform MaterialInfo Material;

layout(location=0) out vec4 FragColor;

vec3 BlinnPhong(vec3 pos, vec3 n) {
    vec3 texColor = texture(Tex1, TexCoord).rgb;
    vec3 amb = Light.La * texColor;
    vec3 s = normalize(Light.Position.xyz - pos);
    float sDotN = max(dot(s, n), 0.0);
    vec3 dif = texColor * sDotN;

    vec3 spec = vec3(0.0);
    if (sDotN > 0.0) {
        vec3 v = normalize(-pos.xyz);
        vec3 h = normalize(v + s);
        spec = Material.Ks * pow(max(dot(h, n), 0.0), Material.Shininess);
    }
    return amb + Light.L * (dif + spec);
}

void main() {
    FragColor = vec4(BlinnPhong(Position, normalize(Normal)), 1.0);
}
