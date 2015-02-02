#version 130

in vec3 vert_pos;
in vec2 vert_uv;
in vec3 vert_normal;
in vec3 vert_tangent;
in vec3 vert_bitangent;

out vec2 UV;
out vec3 EyeDirection_tangentspace;
out vec3 LightDirection_tangentspace;

uniform mat4 mvp;
uniform mat4 view;
uniform mat4 model;
uniform mat3 model_view3;
uniform vec3 light_pos;

void main() {
    gl_Position = mvp * vec4(vert_pos, 1);
    UV = vert_uv;

    vec3 vert_normal_cameraspace = model_view3 * normalize(vert_normal);
    vec3 vert_tangent_cameraspace = model_view3 * normalize(vert_tangent);
    vec3 vert_bitangent_cameraspace = model_view3 * normalize(vert_bitangent);
    mat3 TBN = transpose(mat3(vert_tangent_cameraspace, vert_bitangent_cameraspace, vert_normal_cameraspace));

    LightDirection_tangentspace = TBN * (-light_pos);
    EyeDirection_tangentspace = TBN * (-(view * model * vec4(vert_pos, 1)).xyz);
}
