#pragma once
#include <SDL3/SDL.h>

class Display
{

public:
	Display();
	int Init();
	int CreateWindowChip();
	void DestroyWindow();
	void Update( bool& quit );

private:
	SDL_Window* window;
	SDL_Renderer* renderer;
};
