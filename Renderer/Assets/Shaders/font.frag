#version 450

layout(location=0) in vec2 uv_coords;
layout(location=1) in vec3 frag_color;

layout(location=0) out vec4 out_color;

layout(binding=0) uniform sampler2D text;

void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, uv_coords).r);
    out_color = vec4(frag_color, 1.0) * sampled;
}
