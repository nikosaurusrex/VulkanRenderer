#version 450

layout(binding=0) readonly buffer FontData {
    mat4 proj_matrix;
    vec3 color;
    // xy = position, zw = uv
    vec4 vertices[];
};

layout(location=0) out vec2 uv_coords;
layout(location=1) out vec3 frag_color;

void main() {
    vec4 vertex = vertices[gl_VertexIndex];

    gl_Position = proj_matrix * vec4(vertex.xy, 0.0, 1.0);
    uv_coords = vertex.zw;
    frag_color = color;
}
