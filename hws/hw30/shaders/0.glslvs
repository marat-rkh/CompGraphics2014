#version 130

in vec3 vert_pos_modelspace;
in vec2 vert_uv;
in vec3 vert_normal_modelspace;

out vec2 UV;
out vec3 Position_worldspace;
out vec3 Normal_cameraspace;
out vec3 EyeDirection_cameraspace;
out vec3 LightDirection_cameraspace;

uniform mat4 mvp;
uniform mat4 view;
uniform mat4 model;

uniform vec3 lightpos_worldspace;
uniform float tex_coords_scale;

void main() {
    gl_Position =  mvp * vec4(vert_pos_modelspace, 1);

    Position_worldspace = (model * vec4(vert_pos_modelspace, 1)).xyz;

    mat4 modelview = view * model;
    vec3 vertexPosition_cameraspace = (modelview * vec4(vert_pos_modelspace, 1)).xyz;
    EyeDirection_cameraspace = vec3(0,0,0) - vertexPosition_cameraspace;

    vec3 LightPosition_cameraspace = (view * vec4(lightpos_worldspace, 1)).xyz;
    LightDirection_cameraspace = LightPosition_cameraspace + EyeDirection_cameraspace;

    Normal_cameraspace = (modelview * vec4(vert_normal_modelspace, 0)).xyz;

    UV = tex_coords_scale * vert_uv;
}
