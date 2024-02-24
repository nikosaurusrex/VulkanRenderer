#include "FontRenderer.h"

#include "glm/gtc/matrix_transform.hpp"

Font::Font(const char *font, u32 font_size) {
    if (FT_Init_FreeType(&ft)) {
        LogFatal("Failed to initialize font system");
    }

    if (FT_New_Face(ft, font, 0, &face)) {
        LogFatal("Failed to load font");
    }

    FT_Set_Pixel_Sizes(face, 0, font_size);

    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    for (s32 c = 0; c < 128; c++) {
        // load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            LogFatal("Failed to load glyph");
        }

        u32 texture;
        /*
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        */

        Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                u32(face->glyph->advance.x)
        };

        characters.insert({c, character});
    }

    font_height = characters['H'].size.y;

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

u32 Font::CalculateTextWidth(string text, f32 scale) {
    u32 width = 0;

    for (s32 i = 0; i < text.length(); ++i) {
        char c = text[i];
        Character ch = characters[s32(c)];

        width += ch.size.x * scale;
    }

    return width;
}

FontRenderer::FontRenderer(Font *font) : font(font) {
    /*
    shader = new Shader("engine/assets/shaders/font_vert.glsl", "engine/assets/shaders/font_frag.glsl");
	projection = shader->GetUniform("proj_matrix");
	text_color = shader->GetUniform("text_color");

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);*/
}

FontRenderer::~FontRenderer() {
    /*
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);

    delete shader;*/
}

void FontRenderer::Resize(u32 width, u32 height) {
	glm::mat4 projection_matrix = glm::orthoLH(0.0f, f32(width), f32(height), 0.0f, -1.0f, 1.0f);

    // shader->Use();
	// projection->Set(projection_matrix);
}

void FontRenderer::Render(string text, f32 x, f32 y, f32 scale, glm::vec3 color) {
    /*
    shader->Use();
	text_color->Set(color);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(vao); */

    for (s32 i = 0; i < text.length(); ++i) {
        char c = text[i];
        Character ch = font->characters[s32(c)];

        float xpos = x + ch.bearing.x * scale;
        float ypos = y + (font->characters['H'].bearing.y - ch.bearing.y) * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;

        // update VBO for each character
        float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 0.0f },
                { xpos,     ypos,       0.0f, 0.0f },

                { xpos,     ypos + h,   0.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 0.0f }
        };
        /*
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.texture_id);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);*/
        // now advance cursors for next glyph
        x += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (1/64th times 2^6 = 64)
    }
    /*
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0); */
}

