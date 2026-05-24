#include "MouseHandler.h"
#include "player/input/ITurnInput.h"

#ifdef RPI
#include <SDL/SDL.h>
#endif
#if defined(MACOS) || defined(LINUX)
#include <SDL.h>
#endif
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
extern bool g_win32MouseCaptured;
extern HWND g_win32Hwnd;
#endif

MouseHandler::MouseHandler( ITurnInput* turnInput )
:_turnInput(turnInput)
{}

MouseHandler::MouseHandler()
:_turnInput(0)
{}

MouseHandler::~MouseHandler() {
}

void MouseHandler::setTurnInput( ITurnInput* turnInput ) {
_turnInput = turnInput;
}

void MouseHandler::grab() {
xd = 0;
yd = 0;

#if defined(RPI)
//LOGI("Grabbing input!\n");
SDL_WM_GrabInput(SDL_GRAB_ON);
SDL_ShowCursor(0);
#elif defined(MACOS) || defined(LINUX)
SDL_SetRelativeMouseMode(SDL_TRUE);
SDL_ShowCursor(0);
#elif defined(_WIN32)
g_win32MouseCaptured = true;
ShowCursor(FALSE);
if (g_win32Hwnd) {
RECT r;
GetClientRect(g_win32Hwnd, &r);
MapWindowPoints(g_win32Hwnd, NULL, (LPPOINT)&r, 2);
ClipCursor(&r);
}
#elif defined(EMSCRIPTEN)
// Allow the next canvas mousedown to request pointer lock
EM_ASM({ window.mcMenuOpen = false; });
#endif
}

void MouseHandler::release() {
#if defined(RPI)
//LOGI("Releasing input!\n");
SDL_WM_GrabInput(SDL_GRAB_OFF);
SDL_ShowCursor(1);
#elif defined(MACOS) || defined(LINUX)
SDL_SetRelativeMouseMode(SDL_FALSE);
SDL_ShowCursor(1);
#elif defined(_WIN32)
g_win32MouseCaptured = false;
ShowCursor(TRUE);
ClipCursor(NULL);
#elif defined(EMSCRIPTEN)
// Suppress pointer lock and release any active lock
EM_ASM({
window.mcMenuOpen = true;
if (document.exitPointerLock) document.exitPointerLock();
});
#endif
}

void MouseHandler::poll() {
if (_turnInput != 0) {
TurnDelta td = _turnInput->getTurnDelta();
xd = td.x;
yd = td.y;
}
}
