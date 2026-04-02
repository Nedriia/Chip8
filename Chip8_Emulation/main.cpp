#include "Chip8.h"
#include <windows.h>
#include "Display.h"
#include <chrono>
#include "Input.h"
#include "SoundManager.h"

int Quit()
{
	//TODO : create access key for destroy and Init
	Display::KeyDisplayAccess oKeyDisplay;

	Input::GetInstance()->DestroyInputManager();
	Display::GetInstance()->DestroyWindow( oKeyDisplay );
	SoundManager::GetInstance()->DestroySoundManager();
	Chip8::GetInstance()->DestroyCpu();

	return -1;
}

int main( int argc,char* argv[] )
{
	const char* sROMToLoad = nullptr;
	if( argc >= 1 )
		sROMToLoad = argv[ 1 ];

	Chip8::KeyAccess oKey;
	Display::KeyDisplayAccess oKeyDisplay;

	Chip8::GetInstance()->Init( oKey,sROMToLoad );
	if( Display::GetInstance()->Init( oKeyDisplay,Chip8::GetInstance() ) == -1 )
		return Quit();

	SoundManager::GetInstance()->Init();

	//Loop
	bool quit = false;

	while( !quit )
	{
		std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();

		Chip8::GetInstance()->EmulateCycle( oKey,time );
		Input::GetInstance()->ProcessInput( time,quit );
		Display::GetInstance()->Update( time,Chip8::GetInstance()->IsPause() );
	}

	Quit();

	return 0;
}