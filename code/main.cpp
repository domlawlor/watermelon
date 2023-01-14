#include "defines.h"

#include "game.h"

#include "raylib.h"
#include "raymath.h"

#include "optick/optick.h"

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

#if defined(PLATFORM_WEB)
static void UpdateAndDrawWeb(void *context)
{
	Game *game = (Game *)context;
	game->UpdateAndDraw();
}
#endif


int main(void)
{
	OPTICK_START_CAPTURE();

	TraceLog(LOG_INFO, "WorkDir = %s", GetWorkingDirectory());
	
	const u32 windowWidth = 1600;
	const u32 windowHeight = 900;

	InitWindow(windowWidth, windowHeight, "pineapple");

	SetWindowPosition(1000, 200); // TODO: DEBUG - Remove 

	constexpr s32 targetFPS = 60;
	SetTargetFPS(targetFPS);

	InitAudioDevice();

	//Font font = LoadFont("mecha.png");

	//Music music = LoadMusicStream("ambient.ogg");
	//SetMusicVolume(music, 1.0f);
	//PlayMusicStream(music);

	Game game = Game(windowWidth, windowHeight);

	HideCursor();
	DisableCursor();

	PollInputEvents(); // Called so mouseDelta is correct first frame

#if defined(PLATFORM_WEB)
	emscripten_set_main_loop_arg(UpdateAndDrawWeb, (void *)&game, targetFPS, 1);
#else
	while(!WindowShouldClose())
	{
		OPTICK_FRAME("MainThread");

		game.UpdateAndDraw();
	}
#endif
	
	
	//UnloadFont(font);
	//UnloadMusicStream(music);

	CloseAudioDevice();
	CloseWindow();

	OPTICK_STOP_CAPTURE();
	OPTICK_SAVE_CAPTURE("profileCapture.opt");

	return 0;
}