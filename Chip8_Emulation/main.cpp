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
	Chip8* m_pCpuInstance = Chip8::GetInstance();
	Display* m_pDisplayInstance = Display::GetInstance();
	Input* m_pInputInstance = Input::GetInstance();

	if( m_pDisplayInstance->Init( oKeyDisplay,m_pCpuInstance ) == -1 )
		return Quit();
	m_pCpuInstance->Init( oKey,sROMToLoad );
	SoundManager::GetInstance()->Init();

	//Loop
	bool quit = false;

	while( !quit )
	{
		std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();

		m_pCpuInstance->EmulateCycle( oKey,time );
		m_pInputInstance->ProcessInput( time,quit );
		m_pDisplayInstance->Update( time,m_pCpuInstance->IsPause() );
	}

	Quit();

	return 0;
}