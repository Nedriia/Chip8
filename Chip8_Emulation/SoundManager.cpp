#include "SoundManager.h"
#include <iostream>

#define MINIAUDIO_IMPLEMENTATION
#include "MiniAudio/miniaudio.h"

#define DEVICE_FORMAT		ma_format_f32
#define DEVICE_CHANNELS		2
#define DEVICE_SAMPLE_RATE  48000

SoundManager* SoundManager::m_pSingleton = nullptr;
ma_waveform* SoundManager::squareWave = nullptr;

bool SoundManager::m_bPlaySound = false;

SoundManager::SoundManager() :
	device( nullptr )
{
}

SoundManager::~SoundManager()
{
	SoundManager::squareWave = nullptr;
	m_pSingleton = nullptr;
}

void SoundManager::DestroySoundManager()
{
	if( ma_device_is_started( &device ) )
		ma_device_stop( &device );

	ma_device_uninit( &device );
	delete SoundManager::squareWave;
	delete m_pSingleton;
}

void SoundManager::Manage( const uint8_t soundTimer )
{
	if( soundTimer > 0 )
		Play_Sound();
	else
		Stop_Sound();
}

static void data_callback( ma_device* pDevice,void* pOutput,const void* pInput,ma_uint32 frameCount )
{
	SoundManager::squareWave = ( ma_waveform* )pDevice->pUserData;
	if( SoundManager::m_bPlaySound && SoundManager::squareWave != nullptr )
		ma_waveform_read_pcm_frames( SoundManager::squareWave,pOutput,frameCount,NULL );
	else
		ma_silence_pcm_frames( pOutput, frameCount,DEVICE_FORMAT,DEVICE_CHANNELS );

	( void* )pInput;
}

void SoundManager::Init()
{
	SoundManager::squareWave = new ma_waveform;

	ma_device_config deviceConfig = ma_device_config_init( ma_device_type_playback );
	deviceConfig.playback.format = DEVICE_FORMAT;
	deviceConfig.playback.channels = DEVICE_CHANNELS;
	deviceConfig.sampleRate = DEVICE_SAMPLE_RATE;
	deviceConfig.dataCallback = data_callback;
	deviceConfig.pUserData = SoundManager::squareWave;

	if( ma_device_init( NULL,&deviceConfig,&device ) != MA_SUCCESS )
	{
		std::cerr << "SOUNDMANAGER::FAILED_TO_INITIALIZE_DEVICE" << std::endl;
		return;
	}

	ma_waveform_config squareWaveConfig = ma_waveform_config_init( device.playback.format,device.playback.channels,device.sampleRate,ma_waveform_type_square,0.1,220 );
	ma_waveform_init( &squareWaveConfig,SoundManager::squareWave );

	if( ma_device_start( &device ) != MA_SUCCESS )
	{
		std::cerr << "SOUNDMANAGER::FAILED_TO_START_PLAYBACK_DEVICE" << std::endl;
		ma_device_uninit( &device );
		return;
	}
}

void SoundManager::Play_Sound()
{
	m_bPlaySound = true;
}

void SoundManager::Stop_Sound()
{
	m_bPlaySound = false;
}
