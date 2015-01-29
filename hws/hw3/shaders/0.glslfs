#version 130

in vec2 UV;
in vec3 Position_worldspace;
in vec3 Normal_cameraspace;
in vec3 Tangent_cameraspace;
in vec3 EyeDirection_cameraspace;
in vec3 LightDirection_cameraspace;

out vec3 color;

uniform vec3 lightpos_worldspace;
uniform sampler2D texture_sampler;
uniform vec3 light_color;
uniform float light_power;
uniform vec3 ambient;
uniform vec3 specular;

void main() {
    vec3 MaterialDiffuseColor = texture2D(texture_sampler, UV).rgb;
    vec3 MaterialAmbientColor = ambient * MaterialDiffuseColor;
    vec3 MaterialSpecularColor = specular;

    float distance = length(lightpos_worldspace - Position_worldspace);

    vec3 n = normalize(Normal_cameraspace);
    vec3 l = normalize(LightDirection_cameraspace);
    float cosTheta = clamp(dot(n, l), 0, 1);

    vec3 E = normalize(EyeDirection_cameraspace);
    vec3 R = reflect(-l,n);
    float cosAlpha = clamp(dot(E, R), 0, 1);

    color = MaterialAmbientColor +
            MaterialDiffuseColor * light_color * light_power * cosTheta / (distance*distance) +
            MaterialSpecularColor * light_color * light_power * pow(cosAlpha,5) / (distance*distance);
}
