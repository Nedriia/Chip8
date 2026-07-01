#include "Chip8.h"
#include "Display.h"
#include <chrono>
#include "Input.h"
#include "SoundManager.h"
#include "Chip8_Debugger.h"
#include "TimeManager.h"

int Quit()
{
	Display::KeyDisplayAccess oKeyDisplay;

	Input::GetInstance()->DestroyInputManager();
	SoundManager::GetInstance()->DestroySoundManager();

#ifdef DEBUG_INFO
	Chip8_Debugger::GetInstance()->Destroy();
#endif

	Display::GetInstance()->DestroyWindow( oKeyDisplay );
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

	if( m_pDisplayInstance->Init( oKeyDisplay,m_pCpuInstance ) != 0 )
	{
		Quit();
		return -1;
	}

	m_pCpuInstance->Init( oKey,sROMToLoad );
	SoundManager::GetInstance()->Init();

	bool quit = false;
	while( !quit )
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

		//Emulator main loop
		m_pCpuInstance->EmulateCycle( oKey );
		m_pInputInstance->ProcessInput(quit );
		m_pDisplayInstance->Update( m_pCpuInstance->IsPause() );

		TimeManager::HandleTime( start );
	}

	Quit();
	return 0;
}