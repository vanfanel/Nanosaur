#include <SDL.h>
#include <PommeInternal.h>
#include "GamePatches.h"
#include "GLBackdrop.h"

#include <memory>

extern "C" {
#include "game/Structs.h"
#include "game/windows_nano.h"

extern SDL_Window* gSDLWindow;
extern PrefsType gGamePrefs;
}

constexpr const bool ALLOW_BACKDROP_TEXTURE = true;
std::unique_ptr<GLBackdrop> glBackdrop = nullptr;
static bool backdropPillarbox = false;

static SDL_GLContext exclusiveGLContext = nullptr;
static bool exclusiveGLContextValid = false;

void SetWindowGamma(int percent)
{
	SDL_SetWindowBrightness(gSDLWindow, percent/100.0f);
}

static UInt32* GetBackdropPixPtr()
{
	return (UInt32*)GetPixBaseAddr(GetGWorldPixMap(Pomme::Graphics::GetScreenPort()));
}

static bool IsBackdropAllocated()
{
	return glBackdrop != nullptr;
}

void AllocBackdropTexture()
{
	if (!ALLOW_BACKDROP_TEXTURE
		|| IsBackdropAllocated())
	{
		return;
	}

	glBackdrop = std::make_unique<GLBackdrop>(
		GAME_VIEW_WIDTH,
		GAME_VIEW_HEIGHT,
		(unsigned char *)GetBackdropPixPtr());
	
	Pomme_SetPortDirty(false);
}

void DisposeBackdropTexture()
{
	if (!ALLOW_BACKDROP_TEXTURE
		|| !IsBackdropAllocated())
	{
		return;
	}

	glBackdrop.reset(nullptr);
}

void EnableBackdropPillarboxing(Boolean pillarbox)
{
	backdropPillarbox = pillarbox;
}

void RenderBackdropQuad()
{
	if (!ALLOW_BACKDROP_TEXTURE
		|| !IsBackdropAllocated())
	{
		return;
	}
	
	int windowWidth, windowHeight;
	GLint vp[4];

	SDL_GetWindowSize(gSDLWindow, &windowWidth, &windowHeight);
	glGetIntegerv(GL_VIEWPORT, vp);

	glDisable(GL_SCISSOR_TEST);

	if (Pomme_IsPortDirty()) {
		glBackdrop->UpdateTexture();
		Pomme_SetPortDirty(false);
	}

	glBackdrop->UpdateQuad(windowWidth, windowHeight, backdropPillarbox);

	glBackdrop->Render(windowWidth, windowHeight, backdropPillarbox, gGamePrefs.highQualityTextures);

	if (exclusiveGLContextValid) { // in exclusive GL mode, force swap buffer
		SDL_GL_SwapWindow(gSDLWindow);
	}

	glEnable(GL_SCISSOR_TEST);
	glViewport(vp[0], vp[1], (GLsizei)vp[2], (GLsizei)vp[3]);
}

void ExclusiveOpenGLMode_Begin()
{
	if (!ALLOW_BACKDROP_TEXTURE)
		return;

	if (exclusiveGLContextValid)
		throw std::runtime_error("already in exclusive GL mode");

	exclusiveGLContext = SDL_GL_CreateContext(gSDLWindow);
	exclusiveGLContextValid = true;
	SDL_GL_MakeCurrent(gSDLWindow, exclusiveGLContext);

	AllocBackdropTexture();
}

void ExclusiveOpenGLMode_End()
{
	if (!ALLOW_BACKDROP_TEXTURE)
		return;

	if (!exclusiveGLContextValid)
		throw std::runtime_error("not in exclusive GL mode");

	DisposeBackdropTexture();

	exclusiveGLContextValid = false;
	SDL_GL_DeleteContext(exclusiveGLContext);
	exclusiveGLContext = nullptr;
}
