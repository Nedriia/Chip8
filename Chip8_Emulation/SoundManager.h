#pragma once
#include <cstdint>

#include "MiniAudio/miniaudio.h"

class alignas ( 16 ) SoundManager
{

public:

	void Init();
	void DestroySoundManager();
	void Manage( const uint8_t iSoundTimer );
	void LoadPatternInSoundBuffer( const uint8_t* aAudioPattern );
	void CalculateAndSetNewPitch( const uint8_t iXValue );
	void ClearAudioBuffer();
	void OnReset();
	void SetFloatingIndex( float fFloatingIndex ) { m_fFloatingIndex = fFloatingIndex; }

	float GetFloatingIndex() const { return m_fFloatingIndex; }
	float GetPitch() const { return m_iPitch; }

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
	ma_device				m_oDevice;

	float					m_aSoundBuffer[128] = { 0 };

	int						m_iPitch;
	float					m_fFloatingIndex;
};

