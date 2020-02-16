#include <SDL_opengl.h>
#include "windows_nano.h"

extern UInt32* gCoverWindowPixPtr;

static GLuint backdropTexture = -1;
static bool backdropTextureAllocated = false;

void AllocBackdropTexture()
{
	if (backdropTextureAllocated)
		return;
	
	backdropTextureAllocated = true;
	
	glGenTextures(1, &backdropTexture);
	if (glGetError() != 0)
		TODOFATAL2("couldn't alloc backdrop texture");

	glBindTexture(GL_TEXTURE_2D, backdropTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, gCoverWindowPixPtr);
	Pomme_SetPortDirty(false);
}

void DisposeBackdropTexture()
{
	printf("[BACKDROP] disposed\n");
	
	if (backdropTextureAllocated) {
		glDeleteTextures(1, &backdropTexture);
		backdropTextureAllocated = false;
	}
}

void RenderBackdropQuad()
{
	if (!backdropTextureAllocated || !glIsTexture(backdropTexture))
		return;
	
	GLint vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);
	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);
	glEnable(GL_TEXTURE_2D);
	glViewport(0, 0, GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0, GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT, 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glClearColor(0.0, 0.0, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, backdropTexture);

	if (Pomme_IsPortDirty())
	{
		glTexSubImage2D(GL_TEXTURE_2D,
			0, 0, 0, GAME_VIEW_WIDTH, GAME_VIEW_HEIGHT,
			GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, gCoverWindowPixPtr);
		Pomme_SetPortDirty(false);
	}

	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0.0, 0.0); glVertex2i(0, 0);
	glTexCoord2f(1.0, 0.0); glVertex2i(640, 0);
	glTexCoord2f(0.0, 1.0); glVertex2i(0, 480);
	glTexCoord2f(1.0, 1.0); glVertex2i(640, 480);
	glEnd();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();
	glEnable(GL_SCISSOR_TEST);

	glViewport(vp[0], vp[1], (GLsizei)vp[2], (GLsizei)vp[3]);
}