#pragma once

enum
{
	BACKDROP_FILL = 0,
	BACKDROP_PILLARBOX = 1,
	BACKDROP_LETTERBOX = 2,
	BACKDROP_FIT = BACKDROP_PILLARBOX | BACKDROP_LETTERBOX,
};

#ifdef __cplusplus

#include "GLFunctions.h"
#include <array>

class GLBackdrop
{
	GLFunctions gl;
	GLuint vao;
	GLuint vbo;
	GLuint program;
	GLuint texture;
	int textureWidth;
	int textureHeight;
	unsigned char* textureData;
	int clipWidth;
	int clipHeight;
	bool needClear;
	std::array<GLfloat, 4 * 6> vertexBufferData;

public:
	GLBackdrop(int width, int height, unsigned char* pixels);
	~GLBackdrop();
	void UpdateTexture();
	void UpdateQuad(int windowWidth, int windowHeight, int fit);
	void Render(int windowWidth, int windowHeight, bool linearFiltering);
	void SetClipRegion(int clipWidth, int clipHeight);
};

#endif
