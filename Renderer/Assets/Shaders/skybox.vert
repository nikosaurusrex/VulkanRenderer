#version 450

layout(binding=0) readonly buffer SkyboxData {
    mat4 proj_matrix;
    mat4 view_matrix;
    vec3 positions[];
};

layout(location=0) out vec3 tex_coords;

void main() {
    vec3 pos = positions[gl_VertexIndex];

	vec4 p = proj_matrix * view_matrix * vec4(pos, 1.0);

	gl_Position = p.xyww;

	tex_coords = pos;
}
