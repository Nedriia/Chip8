#include "Display.h"

SDL_Event event;

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 320

Display::Display() :
	window( nullptr ),
	renderer( nullptr )
{
}

int Display::Init()
{
	if( SDL_Init( SDL_INIT_VIDEO ) == false )
	{
		SDL_LogError( SDL_LOG_CATEGORY_ERROR, "Failed to initialize the SDL library: %s\n", SDL_GetError() );
		return -1;
	}

	if( CreateWindowChip() == -1 )
		return -1;

	return 0;
}

int Display::CreateWindowChip()
{
	if( !SDL_CreateWindowAndRenderer( "Chi8 Emulation", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer ) ) {
		SDL_LogError( SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window and renderer: %s", SDL_GetError() );
		return -1;
	}

	if( window == nullptr )
	{
		// In the case that the window could not be made...
		SDL_LogError( SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError() );
		return -1;
	}

	return 0;
}

void Display::DestroyWindow()
{
	SDL_DestroyRenderer( renderer );
	SDL_DestroyWindow( window );

	SDL_Quit();
}

void Display::Update( bool& quit )
{
	SDL_PollEvent( &event );
	if( event.type == SDL_EVENT_QUIT ) {
		quit = true;
	}

	SDL_SetRenderDrawColor( renderer, 0x00, 0x00, 0x00, 0x00 );
	SDL_RenderClear( renderer );
	SDL_RenderPresent( renderer );
}
