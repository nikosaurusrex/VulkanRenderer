#ifndef FONT_RENDERER_H
#define FONT_RENDERER_H

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include <glm/glm.hpp>

#include "Common.h"

struct Character {
    u32 texture_id;
    glm::ivec2 size;
    glm::ivec2 bearing;
    u32 advance;
};

struct Font {
    FT_Library ft;
    FT_Face face;
    map<char, Character> characters;
    u32 font_height;

    Font(const char *font, u32 font_size);

    u32 CalculateTextWidth(string text, f32 scale=1.0f);
};

struct FontRenderer {
	Font *font;
	u32 vao;
	u32 vbo;

	FontRenderer(Font *font);
	~FontRenderer();

	void Resize(u32 width, u32 height);
	void Render(string text, f32 x, f32 y, f32 scale, glm::vec3 color);
};

#endif
