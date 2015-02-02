#version 130

in vec2 UV;
in vec3 LightDirection_tangentspace;
in vec3 EyeDirection_tangentspace;

out vec3 color;

uniform vec3 light_color;
uniform vec3 specular;
uniform vec3 ambient;

uniform float power;
uniform float specular_power;

uniform sampler2D texture_sampler;
uniform sampler2D normals_map_sampler;

void main() {
    vec3 texture_color = texture(texture_sampler, UV).rgb;

    vec3 ambient_part = ambient * texture_color;

    vec3 n = normalize(texture(normals_map_sampler, UV).rgb * 2.0 - 1.0);
    vec3 l = normalize(LightDirection_tangentspace);
    vec3 diffuse_part = clamp(dot(n, l), 0, 1) * texture_color * light_color * power;

    vec3 E = normalize(EyeDirection_tangentspace);
    vec3 R = reflect(-l, n);
    float cosAlpha = clamp((dot(E, R)), 0, 1);
    vec3 specular_part = specular * pow(cosAlpha, specular_power);

    color = ambient_part + diffuse_part + specular_part;
}
