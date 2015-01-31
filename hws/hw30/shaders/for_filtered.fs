#version 130

in vec2 UV;

out vec3 color;

uniform sampler2D texture_sampler;

uniform int filter_type;
const int NO_FILTER = 0;
const int BOX_BLUR = 1;
const int GAUSSIAN_BLUR = 2;
const int SOBEL_FILTER = 3;

const float step_factor = 800;

uniform float gaus_variance;
uniform int gaus_radius;

const float PI = 3.14159265358979323846264;

void box_blur() {
    vec3 sum = vec3(0, 0, 0);
    for(int i = -1; i <= 1; ++i) {
        for(int j = -1; j <= 1; ++j) {
            sum += texture2D(texture_sampler, vec2(UV.x + i / step_factor, UV.y + j / step_factor)).rgb;
        }
    }
    color = sum / 9.0f;
}

float gaussian_function(int x, int y) {
    float two_var_sq = 2 * gaus_variance * gaus_variance;
    return 1.0f / PI * two_var_sq * exp(-1 * (x * x + y * y) / two_var_sq);
}

void gaussian_blur() {
    vec3 sum = vec3(0, 0, 0);
    float kernel_sum = 0;
    for(int i = -gaus_radius; i <= gaus_radius; ++i) {
        for(int j = -gaus_radius; j <= gaus_radius; ++j) {
            vec3 tex_color = texture2D(texture_sampler, vec2(UV.x + i / step_factor, UV.y + j / step_factor)).rgb;
            float weight = gaussian_function(i, j);
            kernel_sum += weight;
            sum += tex_color * weight;
        }
    }
    color = sum / kernel_sum;
}

vec3 sobel_filter() {
    return vec3(1,1,0);
}

void main() {
    if(filter_type == BOX_BLUR) {
        box_blur();
    } else if (filter_type == GAUSSIAN_BLUR) {
        gaussian_blur();
    } else if (filter_type == SOBEL_FILTER) {
        color = sobel_filter();
    } else {
        color = texture2D(texture_sampler, UV).rgb;
    }
}
