#include <ft2build.h>
#include "cgmath.h"
#include "cgut.h"
#include FT_FREETYPE_H
#pragma comment(lib, "freetype261d.lib")

// Freetype objects
FT_Library freetype;	// library handler
FT_Face freetype_face;	// face handler
GLuint VAO, VBO;		// vertex arrays for text objects
GLuint program_text;	// GPU program for rendering text

static const char* font_path = "C:/Windows/Fonts/courbd.ttf";
static const char* vert_text_path = "../bin/shaders/text.vert"; // text vertex shaders
static const char* frag_text_path = "../bin/shaders/text.frag"; // text fragment shaders

struct ft_char_t
{
	GLuint	textureID;	// ID handle of the glyph texture
	ivec2	size;		// Size of glyph
	ivec2	bearing;	// Offset from baseline to left/top of glyph
	GLuint	advance;	// Horizontal offset to advance to next glyph
};
std::map<GLchar, ft_char_t> ft_char_list;

void init_text()
{
	FT_Init_FreeType(&freetype); // initialize FreeType2 handler
	FT_New_Face(freetype, font_path, 0, &freetype_face); // load and create font¡¯s face
	// define face¡¯s pixel size, face will calculate width dynamically when set to 0
	FT_Set_Pixel_Sizes(freetype_face, 0, 48);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	for (GLubyte c = 0; c < 128; c++)
	{
		// Load character glyph
		FT_Load_Char(freetype_face, c, FT_LOAD_RENDER);
		// Generate texture
		GLuint texture_text;

		glGenTextures(1, &texture_text);
		glBindTexture(GL_TEXTURE_2D, texture_text);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			freetype_face->glyph->bitmap.width,
			freetype_face->glyph->bitmap.rows,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			freetype_face->glyph->bitmap.buffer
		);
		// Set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Now store character for later use
		ft_char_t character =
		{
			texture_text,
			ivec2(freetype_face->glyph->bitmap.width, freetype_face->glyph->bitmap.rows),
			ivec2(freetype_face->glyph->bitmap_left, freetype_face->glyph->bitmap_top),
			(GLuint)freetype_face->glyph->advance.x
		};
		ft_char_list.insert(std::pair<GLchar, ft_char_t>(c, character));
	} // loop end

	FT_Done_Face(freetype_face);
	FT_Done_FreeType(freetype);

	program_text = cg_create_program(vert_text_path, frag_text_path); // create text shader program
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	// Vertices below contain x, y, texcoord x, texcoord y
	GLfloat vertices[6][4] = {
			{ 0, 1, 0.0, 0.0 },
			{ 0, 0, 0.0, 1.0 },
			{ 1, 0, 1.0, 1.0 },
			{ 0, 1, 0.0, 0.0 },
			{ 1, 0, 1.0, 1.0 },
			{ 1, 1, 1.0, 0.0 },
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void render_text(std::string text, GLint _x, GLint _y, GLfloat scale, vec4 color)
{
	// Activate corresponding render state
	extern ivec2 window_size;
	GLfloat x = GLfloat(_x);
	GLfloat y = GLfloat(_y);
	glUseProgram(program_text);
	glUniform4f(glGetUniformLocation(program_text, "textColor"), color.r, color.g, color.b, color.a);
	glActiveTexture(GL_TEXTURE);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	mat4 text_offset = mat4(1 / (window_size.x / 2.0f), 0.0f, 0.0f, -1.0f,
							0.0f, 1 / (window_size.y / 2.0f), 0.0f, 1.0f,
							0.0f, 0.0f, 1.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 1.0f); // in render_text function

	std::string::const_iterator c;
	for (c = text.begin(); c != text.end(); c++)
	{
		ft_char_t ch = ft_char_list[*c];
		mat4 text_size = mat4(scale * float(ch.size.x), 0.0f, 0.0f, 0.0f,
			0.0f, scale * float(ch.size.y), 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		);
		mat4 text_translate = mat4(1.0f, 0.0f, 0.0f, x + scale * float(ch.bearing.x),
			0.0f, 1.0f, 0.0f, -y + scale * float(-(ch.size.y - ch.bearing.y)),
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		);
		mat4 text_matrix = mat4();
		text_matrix = text_translate * text_size * text_matrix;
		text_matrix = text_offset * text_matrix;
		glUniformMatrix4fv(glGetUniformLocation(program_text, "text_matrix"), 1, GL_TRUE, text_matrix);
		// Render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.textureID);
		// Render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (ch.advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
	} // iteration end
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}