#version 130

in vec2 UV;

out vec3 color;

uniform sampler2D texture_sampler;

uniform int filter_type;
const int NO_FILTER = 0;
const int BOX_BLUR = 1;
const int GAUSSIAN_HORIZONTAL_BLUR = 2;
const int GAUSSIAN_VERTICAL_BLUR = 3;
const int SOBEL_FILTER = 4;

const float step_factor = 800;

uniform float gaus_variance;
uniform int gaus_radius;

const float PI = 3.14159265358979323846264;

const int SOBEL_KERNEL_RADIUS = 1;
const int SOBEL_KERNEL_SIZE = 3;
uniform float sobel_threshold;

// box blur
void box_blur() {
    vec3 sum = vec3(0, 0, 0);
    for(int i = -1; i <= 1; ++i) {
        for(int j = -1; j <= 1; ++j) {
            sum += texture2D(texture_sampler, vec2(UV.x + i / step_factor, UV.y + j / step_factor)).rgb;
        }
    }
    color = sum / 9.0f;
}

// gaussian blur
float gaussian_function(int x, int y) {
    float numer = exp(-1 * (x * x + y * y) / (2 * gaus_variance * gaus_variance));
    float denom = gaus_variance * gaus_variance * 2 * PI;
    return numer / denom;
}

float gaussian_function(int x) {
    float numer = exp(-1 * x * x / (2 * gaus_variance * gaus_variance));
    float denom = gaus_variance * sqrt(2 * PI);
    return numer / denom;
}

void gaussian_horizontal_blur() {
    vec3 sum = vec3(0, 0, 0);
    float kernel_sum = 0;
    for(int i = -gaus_radius; i <= gaus_radius; ++i) {
        vec3 tex_color = texture2D(texture_sampler, vec2(UV.x + i / step_factor, UV.y)).rgb;
        float weight = gaussian_function(i);
        kernel_sum += weight;
        sum += tex_color * weight;
    }
    color = sum / kernel_sum;
}

void gaussian_vertical_blur() {
    vec3 sum = vec3(0, 0, 0);
    float kernel_sum = 0;
    for(int i = -gaus_radius; i <= gaus_radius; ++i) {
        vec3 tex_color = texture2D(texture_sampler, vec2(UV.x, UV.y + i / step_factor)).rgb;
        float weight = gaussian_function(i);
        kernel_sum += weight;
        sum += tex_color * weight;
    }
    color = sum / kernel_sum;
}

// sobel filter
int sobel_y_weight[9] = int[9](
    -1, -2, -1,
    0,  0,  0,
    1,  2,  1
);

int sobel_x_weight[9] = int[9](
    -1, 0, 1,
    -2, 0, 2,
    -1, 0, 1
);

float rgb_to_brightness(vec3 rgb) {
    // this magic numbers are widely used to convert rgb to grayscale
    float raw_brightness = rgb[0] * 0.2989 + rgb[1] * 0.5870 + rgb[2] * 0.1140;
    return min(1, raw_brightness);
}

void apply_threshold(vec3 rgb) {
    float brightness = rgb_to_brightness(rgb);
    if(brightness < sobel_threshold) {
        color = vec3(0, 0, 0);
    } else {
        color = vec3(brightness, brightness, brightness);
    }
}

void sobel_filter() {
    vec3 sum_x = vec3(0, 0, 0);
    vec3 sum_y = vec3(0, 0, 0);
    for(int i = -1; i <= 1; ++i) {
        for(int j = -1; j <= 1; ++j) {
            vec3 tex_color = texture2D(texture_sampler, vec2(UV.x + i / step_factor, UV.y + j / step_factor)).rgb;
            int index = (i + SOBEL_KERNEL_RADIUS) * SOBEL_KERNEL_SIZE + (i + SOBEL_KERNEL_RADIUS);
            sum_x += tex_color * sobel_x_weight[index];
            sum_y += tex_color * sobel_y_weight[index];
        }
    }

    vec3 rgb_color = abs(sum_x) + abs(sum_y);
    apply_threshold(rgb_color);
}

void main() {
    if(filter_type == BOX_BLUR) {
        box_blur();
    } else if (filter_type == GAUSSIAN_HORIZONTAL_BLUR) {
        gaussian_horizontal_blur();
    } else if (filter_type == GAUSSIAN_VERTICAL_BLUR) {
        gaussian_vertical_blur();
    } else if (filter_type == SOBEL_FILTER) {
        sobel_filter();
    } else {
        color = texture2D(texture_sampler, UV).rgb;
    }
}
