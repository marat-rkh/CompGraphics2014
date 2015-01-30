#version 130

in vec3 vert_pos_modelspace;
in vec2 vert_uv;

out vec2 UV;

uniform mat4 mvp;

void main() {
    gl_Position =  mvp * vec4(vert_pos_modelspace, 1);

    UV = vert_uv;
}

