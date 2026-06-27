#include "Input.h"
#include <chrono>
#include "Display.h"
#include <iostream>
#include "Chip8.h"

Input* Input::m_pSingleton = nullptr;

Input::Input()
	:
	m_aInputs{ 0 },
	m_iLastTimeUpdate( std::chrono::steady_clock::now() ),
	m_pDisplayInstance( nullptr ),
	m_bEnglishLayout( true ),
	m_bOverride( true )
{
}

Input::~Input()
{
	m_pSingleton = nullptr;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void Input::ProcessInput( const std::chrono::steady_clock::time_point& time,bool& quit )
{
	if( m_pDisplayInstance == nullptr )
	{
		m_pDisplayInstance = Display::GetInstance();
		return;
	}

	GLFWwindow* pWindow = m_pDisplayInstance->GetWindow();
	if( pWindow == nullptr )
		return;

	if( !glfwWindowShouldClose( pWindow ) )
	{
		std::chrono::microseconds elapsed = std::chrono::duration_cast< std::chrono::microseconds >( time - m_iLastTimeUpdate );
		if( elapsed >= Display::GetRefreshTick() )
		{
			if( glfwGetKey( pWindow,GLFW_KEY_ESCAPE ) == GLFW_PRESS )
				glfwSetWindowShouldClose( pWindow,true );

			for( uint8_t i = 0; i < 16; ++i )
				CheckInputState( i,pWindow );

			m_iLastTimeUpdate = time;
		}

		glfwPollEvents();
	}
	else
		quit = true;
}

void Input::DestroyInputManager()
{
	delete m_pSingleton;
}

void Input::InitInputFromDatabase( std::map<std::string,int>& aKeys )
{
	if( m_bOverride )
		InitInputDefault();

	if( aKeys.empty() )
		return;
	else
		m_bOverride = true;
	
	const char* sName = glfwGetKeyName( GLFW_KEY_W,glfwGetKeyScancode( GLFW_KEY_W ) );
	if( sName != nullptr )
	{
		if( tolower( *sName ) == ( int )'z' )
			m_bEnglishLayout = false;
	}

	const char* name = glfwGetKeyName( GLFW_KEY_Z,0 );
	int iValue = aKeys[ "up" ];
	if( iValue )
		m_aKeyMap[ iValue ] = m_bEnglishLayout ? GLFW_KEY_Z : GLFW_KEY_W;

	iValue = aKeys[ "down" ];
	if( iValue )
		m_aKeyMap[ iValue ] = GLFW_KEY_S;

	iValue = aKeys[ "left" ];
	if( iValue )
		m_aKeyMap[ iValue ] = m_bEnglishLayout ? GLFW_KEY_Q : GLFW_KEY_A;

	iValue = aKeys[ "right" ];
	if( iValue )
		m_aKeyMap[ iValue ] = GLFW_KEY_D;

	iValue = aKeys[ "a" ];
	if( iValue )
		m_aKeyMap[ iValue ] = GLFW_KEY_1;

	iValue = aKeys[ "b" ];
	if( iValue )
		m_aKeyMap[ iValue ] = GLFW_KEY_2;

	iValue = aKeys[ "player2Up" ];
	if( iValue )
		m_aKeyMap[ iValue ] = GLFW_KEY_I;

	iValue = aKeys[ "player2Down" ];
	if( iValue )
		m_aKeyMap[ iValue ] = GLFW_KEY_K;
}

void Input::InitInputDefault()
{
	m_aKeyMap[ 0x0 ] = GLFW_KEY_0;
	m_aKeyMap[ 0x1 ] = GLFW_KEY_1;
	m_aKeyMap[ 0x2 ] = GLFW_KEY_2;
	m_aKeyMap[ 0x3 ] = GLFW_KEY_3;
	m_aKeyMap[ 0x4 ] = GLFW_KEY_4;
	m_aKeyMap[ 0x5 ] = GLFW_KEY_5;
	m_aKeyMap[ 0x6 ] = GLFW_KEY_6;
	m_aKeyMap[ 0x7 ] = GLFW_KEY_7;
	m_aKeyMap[ 0x8 ] = GLFW_KEY_8;
	m_aKeyMap[ 0x9 ] = GLFW_KEY_9;
	m_aKeyMap[ 0xA ] = GLFW_KEY_A;
	m_aKeyMap[ 0xB ] = GLFW_KEY_B;
	m_aKeyMap[ 0xC ] = GLFW_KEY_C;
	m_aKeyMap[ 0xD ] = GLFW_KEY_D;
	m_aKeyMap[ 0xE ] = GLFW_KEY_E;
	m_aKeyMap[ 0xF ] = GLFW_KEY_F;

	m_bOverride = false;
}

uint8_t Input::GetKeyState( uint8_t iIndex ) const
{
	if ( iIndex >= 0x10 )
		return 0;
	else
		return m_aInputs[ iIndex ];
}

uint8_t Input::IsAnyKeyPress() const
{
	for( int i = 0; i < 16; ++i )
	{
		if( m_aInputs[ i ] )
			return i;
	}

	return 0xFF;
}

void Input::CheckInputState( const uint8_t iKey, GLFWwindow* pWindow )
{
	int iKeyId = m_aKeyMap[ iKey ];
	if( iKeyId )
		m_aInputs[ iKey ] = glfwGetKey( pWindow,iKeyId ) == GLFW_PRESS;
}
