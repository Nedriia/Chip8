#pragma once
#include <cstdint>

#include "MiniAudio/miniaudio.h"

class alignas ( 16 ) SoundManager
{

public:

	void Init();
	void DestroySoundManager();
	void Manage( const uint8_t soundTimer );
	
	static bool m_bPlaySound;
	static ma_waveform* squareWave;

	static SoundManager* GetInstance()
	{
		if( m_pSingleton == nullptr )
			m_pSingleton = new SoundManager;
		return m_pSingleton;
	}

private:

	SoundManager();
	~SoundManager();

	void Play_Sound();
	void Stop_Sound();
	static SoundManager*	m_pSingleton;
	ma_device device;

};

