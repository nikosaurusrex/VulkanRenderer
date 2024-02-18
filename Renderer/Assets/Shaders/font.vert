#version 450

layout (location = 0) in vec4 vertex;

out vec2 uv_coords;

uniform mat4 proj_matrix;

void main() {
    gl_Position = proj_matrix * vec4(vertex.xy, 0.0, 1.0);
    uv_coords = vertex.zw;
}
