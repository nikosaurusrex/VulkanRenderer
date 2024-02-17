#version 330 core

layout (location = 0) in vec2 pos;

out vec2 uv_coord;

uniform mat4 proj_matrix;
uniform vec4 transform;

void main() {
    uv_coord = vec2(pos.x, 1.0 - pos.y);

    vec2 screen_pos = pos * transform.zw + transform.xy;

	gl_Position = proj_matrix * vec4(screen_pos, 0.0, 1.0);
}