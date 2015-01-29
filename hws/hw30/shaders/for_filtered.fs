#version 130

in vec2 UV;

out vec3 color;

uniform sampler2D texture_sampler;

void main() {
    color = texture2D(texture_sampler, UV).rgb;
    // color = vec3(0, 1, 0);
}