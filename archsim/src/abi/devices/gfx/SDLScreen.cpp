/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * abi/devices/gfx/SDLScreen.cpp
 */
#include "abi/devices/gfx/SDLScreen.h"
#include "abi/devices/gfx/VirtualScreen.h"
#include "abi/devices/gfx/VirtualScreenManager.h"

#include "abi/devices/generic/Keyboard.h"
#include "abi/devices/generic/Mouse.h"

#include "concurrent/Thread.h"
#include "util/ComponentManager.h"
#include "util/LogContext.h"

#include "system.h"

#include <unistd.h>
#include <SDL2/SDL.h>

UseLogContext(LogVirtualScreen);
DeclareChildLogContext(LogSDLScreen, LogVirtualScreen, "SDL");

using namespace archsim::abi::devices::gfx;

static const uint32_t sdl_scancode_map[] = {
	0, 0, 0, 0,

	// A - M
	0x1c, 0x32, 0x21, 0x23, 0x24, 0x2b, 0x34, 0x33, 0x43, 0x3b, 0x42, 0x4b, 0x3a,
	// N - Z
	0x31, 0x44, 0x4d, 0x15, 0x2d, 0x1b, 0x2c, 0x3c, 0x2a, 0x1d, 0x22, 0x35, 0x1a,
	// 0 - 9
	0x16, 0x1e, 0x26, 0x25, 0x2e, 0x36, 0x3d, 0x3e, 0x46, 0x45,

	0x5a, 0x76, 0x66, 0x0d, 0x29,
	0x4e, 0x55, 0x54, 0x5b, 0x5d, 0x5d, 0x4c, 0x52, 0x0e, 0x41, 0x49, 0x4a,

	// CAPSLOCK
	0x58,

	// F1 - F12
	0x05, 0x06, 0x04, 0x0c, 0x03, 0x0b, 0x83, 0x0a, 0x01, 0x09, 0x78, 0x07,

	// PRNT SCR, SCRL LCK, PAUSE
	0,           0,        0,

	// INS  HOME    PGUP    DEL     END     PGDN    RT      LT      DN      UP
	0xe070, 0xe06c, 0xe07d, 0xe071, 0xe069, 0xe07a, 0xe074, 0xe06b, 0xe072, 0xe075,

	// NUMLOCK
	0x77,

	// DIV  MUL  MINUS  PLUS  ENTER
	0xe04a, 0x7c, 0x7b, 0x79, 0xe05a,

	// KP 1 - 9, 0
	0x69, 0x72, 0x7a, 0x6b, 0x73, 0x74, 0x6c, 0x75, 0x7d, 0x70,

	// KP .
	0x71,

	// BACKSLASH
	0x5d,				// 100

	// APPS
	0xe02f,
	0, // POWER
	0, // KP EQ
	// Extra F keys
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	// Utility Keys
	/*116*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	/*130*/	0, 0, 0,

	/*133*/	0, // KP COMMA
	0, // SOME WEIRD EQUALS
	// INTERNATIONAL KEYS
	/*135*/	0, 0, 0, 0, 0, 0, 0, 0, 0,
	// LANGUAGE KEYS
	/*144*/	0, 0, 0, 0, 0, 0, 0, 0, 0,

	// OTHER
	/*153*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	/*165*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	/*176*/	0, 0, 0, 0, 0, 0,
	/*182*/	0, 0, 0, 0, 0, 0,
	/*188*/	0, 0, 0, 0, 0, 0,
	/*194*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0,

	// LCTRL LSHIFT LALT  LGUI    RCTRL   RSHIFT RALT    RGUI
	0x14,    0x12,  0x11, 0xe01f, 0xe014, 0x59,  0xe011, 0xe027
};

SDLScreen::SDLScreen(std::string id, memory::MemoryModel *mem_model, System *sys)
	: VirtualScreen(id, mem_model),
	  Thread("SDL Screen"),
	  terminated(false),
	  running(false),
	  window(NULL),
	  renderer(NULL),
	  window_texture(NULL),
	  kbd(NULL),
	  mouse(NULL),
	  hw_accelerated(false),
	  system(sys)
{
}

SDLScreen::~SDLScreen()
{
	if(running) Reset();
}

std::mutex SDLScreen::_sdl_lock;
bool SDLScreen::_sdl_initialised = false;

bool SDLScreen::Initialise()
{
	_sdl_lock.lock();
	if(!_sdl_initialised) {
		LC_DEBUG1(LogSDLScreen) << "Initialising SDL screen";
		SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
		_sdl_initialised = true;
	}
	_sdl_lock.unlock();
	window = SDL_CreateWindow(GetId().c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, GetWidth(), GetHeight(), SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if(!window) {
		LC_ERROR(LogSDLScreen) << "Could not create SDL window! Terminating.";
		return false;
	}

	uint32_t flags = 0;


	//First, try and start a HW accelerated renderer
	if(hw_accelerated)
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	//If we fail to start with HW acceleration, try and get a SW renderer
	if(!renderer) {
		LC_WARNING(LogSDLScreen) << "Could  not create HW Accelerated SDL renderer. Falling back to software.";
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

		if(!renderer) {
			LC_ERROR(LogSDLScreen) << "Attempted but failed to fall back to software renderer. Terminating screen.";
			return false;
		}
	}

	auto mode = SDL_PIXELFORMAT_ABGR1555;
	switch(GetMode()) {
		case VSM_RGBA8888:
			mode = SDL_PIXELFORMAT_ABGR8888;
			break;
		case VSM_16bit:
			mode = SDL_PIXELFORMAT_RGB565;
			break;
		case VSM_8Bit:
			mode = SDL_PIXELFORMAT_RGB332;
			break;
		case VSM_DoomPalette:
			mode = SDL_PIXELFORMAT_RGB888;
			break;
		default:
			LC_ERROR(LogSDLScreen) << "Could not initialise an SDL screen with mode " << GetMode();
			return false;
	}

	window_texture = SDL_CreateTexture(renderer, mode, SDL_TEXTUREACCESS_STREAMING, GetWidth(), GetHeight());
	if(!window_texture) {
		LC_ERROR(LogSDLScreen) << "Could not create window texture! Terminating.";
		return false;
	}
	start();
	LC_DEBUG1(LogSDLScreen) << "SDL Screen brought up";
	return true;
}

bool SDLScreen::Reset()
{
	if(!running) {
		LC_ERROR(LogSDLScreen) << "Attempted to reset a screen while it was not running";
		return false;
	}

	terminate();
	if(window_texture) {
		SDL_DestroyTexture(window_texture);
		window_texture = NULL;
	}
	if(renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = NULL;
	}
	if(window) {
		SDL_DestroyWindow(window);
		window = NULL;
	}

	join();
	return true;
}

void SDLScreen::run()
{
	terminated = false;
	running = true;
	while (!terminated) {
		// Stop the event queue from overflowing
		SDL_Event e;
		_sdl_lock.lock();
		while (SDL_PollEvent(&e)) {
			switch (e.type)	{
				case SDL_KEYDOWN:
					if (kbd) {
						if(e.key.keysym.scancode >= SDL_SCANCODE_F1 && e.key.keysym.scancode <= SDL_SCANCODE_F12) {
							kbd->KeyDown(sdl_scancode_map[SDL_SCANCODE_LCTRL]);
							kbd->KeyDown(sdl_scancode_map[SDL_SCANCODE_LALT]);
						}
						kbd->KeyDown(sdl_scancode_map[e.key.keysym.scancode]);
					}
					break;

				case SDL_KEYUP:
					if (kbd) {
						kbd->KeyUp(sdl_scancode_map[e.key.keysym.scancode]);
						if(e.key.keysym.scancode >= SDL_SCANCODE_F1 && e.key.keysym.scancode <= SDL_SCANCODE_F12) {
							kbd->KeyUp(sdl_scancode_map[SDL_SCANCODE_LCTRL]);
							kbd->KeyUp(sdl_scancode_map[SDL_SCANCODE_LALT]);
						}
					}
					break;

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					if(e.button.state == SDL_PRESSED) {
						switch (e.button.button) {
							case 1:
								mouse->ButtonDown(0);
								break;
							case 2:
								mouse->ButtonDown(2);
								break;
							case 3:
								mouse->ButtonDown(1);
								break;
						}
					} else if(e.button.state == SDL_RELEASED) {
						switch (e.button.button) {
							case 1:
								mouse->ButtonUp(0);
								break;
							case 2:
								mouse->ButtonUp(2);
								break;
							case 3:
								mouse->ButtonUp(1);
								break;
						}
					}

					break;

				case SDL_MOUSEMOTION:
					if (mouse)
						mouse->Move(e.motion.x, -e.motion.y);
					break;

				case SDL_QUIT:
					system->HaltSimulation();
					break;
			}
		}

		// Draw a frame
		draw_frame();

		_sdl_lock.unlock();

		usleep(20000);
	}
	running = false;
	terminated = false;
}

void SDLScreen::terminate()
{
	if(!running) {
		LC_DEBUG1(LogSDLScreen) << "Attempted to terminate screen while it was not running";
		return;
	}
	LC_DEBUG1(LogSDLScreen) << "Terminating screen...";
	terminated = true;
	while(running) ;
	LC_DEBUG1(LogSDLScreen) << "Terminated.";
}

void SDLScreen::draw_frame()
{
	SDL_RenderClear(renderer);

	switch (GetMode()) {
		case VSM_DoomPalette:
			draw_doom();
			break;
		default:
			draw_rgb();
			break;
	}

	bool success = false;
	success = SDL_RenderCopy(renderer, window_texture, NULL, NULL);
	assert(!success);
	SDL_RenderPresent(renderer);
}

bool SDLScreen::draw_rgb()
{
	host_addr_t addr;
	uint32_t pixel_size = 0;
	switch(GetMode()) {
		case VSM_16bit:
			pixel_size = 2;
			break;
		case VSM_RGBA8888:
			pixel_size = 4;
			break;
		default:
			LC_ERROR(LogSDLScreen) << "Tried to draw a frame with an invalid pixel mode " << GetMode();
			return false;
	}

	int pitch = GetWidth() * pixel_size;
	if(!GetMemory()->LockRegion(fb_ptr, GetHeight() * pitch, addr)) {
		LC_ERROR(LogSDLScreen) << "Could not lock region!";
		terminate();
		return false;
	}

	bool success = false;
	success = SDL_UpdateTexture(window_texture, NULL, (void*)addr, pitch);
	assert(!success);

	return true;
}

bool SDLScreen::draw_doom()
{
	host_addr_t fb, palette;

	GetMemory()->LockRegion(fb_ptr, GetWidth() * GetHeight(), fb);
	GetMemory()->LockRegion(p_ptr, GetWidth() * GetHeight(), palette);

	uint8_t *pixels;
	int pitch;

	if (SDL_LockTexture(window_texture, NULL, (void**)&(pixels), &pitch)) {
		LC_ERROR(LogSDLScreen) << "Failed to lock pixels! Terminating.";
		terminate();
		return false;
	}

	for (uint32_t y = 0; y < GetHeight(); y++) {
		for (uint32_t x = 0; x < GetWidth(); x++) {
			uint8_t palette_index = ((uint8_t *)fb)[x + (y * GetWidth())];

			pixels[(x * 4) + (y * pitch)] = ((uint8_t *)palette)[(palette_index * 3) + 2];
			pixels[(x * 4) + (y * pitch) + 1] = ((uint8_t *)palette)[(palette_index * 3) + 1];
			pixels[(x * 4) + (y * pitch) + 2] = ((uint8_t *)palette)[(palette_index * 3) + 0];
		}
	}

	SDL_UnlockTexture(window_texture);
	return true;
}


class SDLScreenManager : public VirtualScreenManager<SDLScreen> {};
RegisterComponent(VirtualScreenManagerBase, SDLScreenManager, "SDL", "Zero-copy SDL-based screen. Uses HW acceleration if available.");
