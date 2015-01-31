#version 130

in vec2 UV;

out vec3 color;

uniform sampler2D texture_sampler;

uniform int filter_type;
const int NO_FILTER = 0;
const int BOX_BLUR = 1;
const int GAUSSIAN_BLUR = 2;
const int SOBEL_FILTER = 3;

void box_blur() {
    const float offset = 1.0f / 800;
    vec3 sum = vec3(0, 0, 0);
    for(int i = -1; i <= 1; ++i) {
        for(int j = -1; j <= 1; ++j) {
            sum += texture2D(texture_sampler, vec2(UV.x + i * offset, UV.y + j * offset)).rgb;
        }
    }
    color = vec3(sum.x / 9.0f, sum.y / 9.0f, sum.z / 9.0f);
}

vec3 gaussian_blur() {
    return vec3(0,1,0);
}

vec3 sobel_filter() {
    return vec3(1,1,0);
}

void main() {
    if(filter_type == BOX_BLUR) {
        box_blur();
    } else if (filter_type == GAUSSIAN_BLUR) {
        color = gaussian_blur();
    } else if (filter_type == SOBEL_FILTER) {
        color = sobel_filter();
    } else {
        color = texture2D(texture_sampler, UV).rgb;
    }
}
