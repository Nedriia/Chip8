#include "SoundManager.h"
#include <iostream>
#include "Chip8.h"

#define MINIAUDIO_IMPLEMENTATION
#include "MiniAudio/miniaudio.h"

#define DEVICE_FORMAT		ma_format_f32
#define SAMPLE_RATE			44100
#define CHUNK_SIZE			128
#define AMPLITUDE			0.2f

static ma_audio_buffer	g_oAudioBuffer;
static ma_waveform		g_oWaveForm;
static bool				g_bPlaySound = false;

static float m_aAudioData[ CHUNK_SIZE ];

SoundManager* SoundManager::m_pSingleton = nullptr;

SoundManager::SoundManager()
	: m_iPitch( 0 )
	 ,m_fFloatingIndex( 0.0f )
	 ,m_iAudioStateFlag( AudioState::AUDIO_BUFFER_EMPTY )
	 ,m_oDevice( nullptr )
{

}

SoundManager::~SoundManager()
{
	m_pSingleton = nullptr;
}

void SoundManager::DestroySoundManager()
{
	if ( ma_device_get_state( &m_oDevice ) != ma_device_state_uninitialized )
	{
		if( ma_device_is_started( &m_oDevice ) )
			ma_device_stop( &m_oDevice );

		ClearAudioBuffer();

		ma_audio_buffer_uninit( &g_oAudioBuffer );
		ma_waveform_uninit( &g_oWaveForm );

		ma_device_uninit( &m_oDevice );
	}

	delete m_pSingleton;
}

void SoundManager::Manage( const uint8_t iSoundTimer )
{
	if( iSoundTimer > 0 )
		Play_Sound();
	else
		Stop_Sound();
}

void SoundManager::LoadPatternInSoundBuffer( const uint8_t* aAudioPattern )
{
	int iIndex = 0;
	for( int i = 0; i < 16; i++ )
	{
		for( int k = 7; k >= 0; --k )
		{
			float fValue = ( aAudioPattern[ i ] >> k ) & 1;
			m_aAudioData[ iIndex ] = fValue == 0.0f ? -AMPLITUDE : std::clamp( fValue, 0.0f, AMPLITUDE );
			++iIndex;
		}
	}
	m_iAudioStateFlag = AudioState::AUDIO_BUFFER_FILLED;
}

void SoundManager::CalculateAndSetNewPitch( const uint8_t iXValue )
{
	m_iPitch = 4000 * exp2( ( iXValue - 64 ) / 48.f );
}

void SoundManager::ClearAudioBuffer()
{
	memset( m_aAudioData, 0, sizeof( float ) * CHUNK_SIZE );
}

void SoundManager::OnReset()
{
	ClearAudioBuffer();

	m_iPitch = SAMPLE_RATE;
	m_fFloatingIndex = 0.0f;
	m_iAudioStateFlag = AudioState::AUDIO_BUFFER_EMPTY;
}

static void data_callback( ma_device* pDevice,void* pOutput,const void* pInput,ma_uint32 frameCount )
{
	bool bPause = !Chip8::GetInstance()->IsRunning();
	if( !bPause )
	{
		SoundManager* pInstance = SoundManager::GetInstance();
		if( pInstance->GetState() == AudioState::AUDIO_BUFFER_FILLED )
		{
		float fValueIncrement = pInstance->GetPitch() / SAMPLE_RATE;
		float* pData = ( float* )pOutput;

		for( ma_uint64 iFrameRead = 0; iFrameRead < frameCount; ++iFrameRead )
		{
			float fCurrentIndex = pInstance->GetFloatingIndex();

			pData[iFrameRead] = m_aAudioData[ ( int )fCurrentIndex ];

			float fNewValue = fCurrentIndex + fValueIncrement;
			if( fNewValue >= 128.0f )
				fNewValue -= 128.0f;
			pInstance->SetFloatingIndex( fNewValue );
		}

			if( g_bPlaySound == false )
				pInstance->SetState( AudioState::AUDIO_BUFFER_EMPTY );
		}
		else
		{
			if( g_bPlaySound )
				ma_waveform_read_pcm_frames( &g_oWaveForm,pOutput,frameCount,NULL );
		}
	}
	else if( bPause || !g_bPlaySound )
	{
		ma_silence_pcm_frames( pOutput,frameCount,DEVICE_FORMAT,1 );
	}

	( void )pInput;
}

void SoundManager::Init()
{
	DISABLE_SPECIFIC_LEAK_DETECTION();
	ClearAudioBuffer();

	ma_waveform_config oSquareWaveConfig = ma_waveform_config_init( DEVICE_FORMAT,1,SAMPLE_RATE,ma_waveform_type_square,AMPLITUDE,220 );
	if( ma_waveform_init( &oSquareWaveConfig,&g_oWaveForm ) != MA_SUCCESS )
		std::cerr << "SOUNDMANAGER::FAILED_TO_INIT_WAVEFORM" << std::endl;

	ma_audio_buffer_config oAudioBufferConfig = ma_audio_buffer_config_init( DEVICE_FORMAT,1,CHUNK_SIZE,m_aAudioData,NULL );
	if( ma_audio_buffer_init( &oAudioBufferConfig,&g_oAudioBuffer ) != MA_SUCCESS )
		std::cerr << "SOUNDMANAGER::FAILED_TO_INIT_AUDIO_BUFFER" << std::endl;

	ma_device_config oDeviceConfig = ma_device_config_init( ma_device_type_playback );
					oDeviceConfig.playback.format = DEVICE_FORMAT;
					oDeviceConfig.playback.channels = 1;
					oDeviceConfig.sampleRate = SAMPLE_RATE;
					oDeviceConfig.dataCallback = data_callback;

	if( ma_device_init( NULL,&oDeviceConfig,&m_oDevice ) != MA_SUCCESS )
	{
		std::cerr << "SOUNDMANAGER::FAILED_TO_INITIALIZE_DEVICE" << std::endl;
		ma_audio_buffer_uninit( &g_oAudioBuffer );
		ma_waveform_uninit( &g_oWaveForm );
		return;
	}
	if( ma_device_start( &m_oDevice ) != MA_SUCCESS )
	{
		std::cerr << "SOUNDMANAGER::FAILED_TO_START_DEVICE" << std::endl;
		ma_audio_buffer_uninit( &g_oAudioBuffer );
		ma_waveform_uninit( &g_oWaveForm );
		ma_device_uninit( &m_oDevice );
		return;
	}
	ENABLE_SPECIFIC_LEAK_DETECTION();
}

void SoundManager::Play_Sound()
{
	g_bPlaySound = true;
}

void SoundManager::Stop_Sound()
{
	g_bPlaySound = false;
}
