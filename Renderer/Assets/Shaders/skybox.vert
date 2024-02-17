#version 330 core

layout (location = 0) in vec3 pos;

out vec3 tex_coords;

uniform mat4 proj_matrix;
uniform mat4 view_matrix;

void main() {
	vec4 p = proj_matrix * view_matrix * vec4(pos, 1.0);

	gl_Position = p.xyww;

	tex_coords = pos;
}
