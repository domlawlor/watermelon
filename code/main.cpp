#include "defines.h"

#include "game.h"

#include "raylib.h"
#include "raymath.h"

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

	Raylib::TraceLog(Raylib::LOG_INFO, "WorkDir = %s", Raylib::GetWorkingDirectory());
	
	const u32 windowWidth = 1600;
	const u32 windowHeight = 900;
	{
		OPTICK_EVENT("InitWindow");

		Raylib::InitWindow(windowWidth, windowHeight, "pineapple");

		Raylib::SetWindowPosition(1000, 200); // TODO: DEBUG - Remove 

		constexpr s32 targetFPS = 60;
		Raylib::SetTargetFPS(targetFPS);
	}

	{
		OPTICK_EVENT("InitAudioDevice");
		Raylib::InitAudioDevice();
	}
	//Raylib::Font font = Raylib::LoadFont("mecha.png");

	//Raylib::Music music = Raylib::LoadMusicStream("ambient.ogg");
	//Raylib::SetMusicVolume(music, 1.0f);
	//Raylib::PlayMusicStream(music);

	Game game = Game(windowWidth, windowHeight);

	Raylib::HideCursor();
	Raylib::DisableCursor();

	Raylib::PollInputEvents(); // Called so mouseDelta is correct first frame

#if defined(PLATFORM_WEB)
	emscripten_set_main_loop_arg(UpdateAndDrawWeb, (void *)&game, targetFPS, 1);
#else
	while(!Raylib::WindowShouldClose())
	{
		OPTICK_FRAME("MainThread");

		game.UpdateAndDraw();
	}
#endif
	
	
	//Raylib::UnloadFont(font);
	//Raylib::UnloadMusicStream(music);

	Raylib::CloseAudioDevice();
	Raylib::CloseWindow();

	OPTICK_STOP_CAPTURE();
	OPTICK_SAVE_CAPTURE("profileCapture.opt");

	return 0;
}