#version 330 core

in vec2 uv_coord;

out vec4 color;

uniform sampler2D tex_sampler;
uniform int has_texture;

uniform vec4 tint = vec4(1.0);

void main() {
    color = tint;

    if (has_texture > 0) {
	    color *= texture(tex_sampler, uv_coord);
	}
}
