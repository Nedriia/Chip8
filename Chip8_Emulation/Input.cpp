#include "Input.h"
#include <chrono>
#include "Display.h"
#include <iostream>

Input* Input::m_pSingleton = nullptr;

Input::Input()
	:
	m_aInputs{ 0 },
	m_iLastTimeUpdate( std::chrono::steady_clock::now() )
{
}

Input::~Input()
{
	m_pSingleton = nullptr;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void Input::ProcessInput( const std::chrono::steady_clock::time_point& time,bool& quit )
{
	if( Display::GetInstance() == nullptr )
		return;

	GLFWwindow* pWindow = Display::GetInstance()->GetWindow();
	if( pWindow == nullptr )
		return;

	if( !glfwWindowShouldClose( pWindow ) )
	{
		std::chrono::microseconds elapsed = std::chrono::duration_cast< std::chrono::microseconds >( time - m_iLastTimeUpdate );
		if( elapsed >= Display::GetRefreshTick() )
		{
			if( glfwGetKey( pWindow,GLFW_KEY_ESCAPE ) == GLFW_PRESS )
				glfwSetWindowShouldClose( pWindow,true );

			for( uint8_t i = 0; i < 0x10; ++i )
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

uint8_t Input::GetKeyState( uint8_t iIndex ) const
{
	if( iIndex >= 0x10 )
	{
		std::cerr << "INPUT::OUT_OF_RANGE" << std::endl;
		return 0;
	}

	return m_aInputs[ iIndex ];
}

uint8_t Input::IsAnyKeyPress() const
{
	for( int i = 0; i < 0x10; ++i )
	{
		if( m_aInputs[ i ] )
			return i;
	}

	return 0xFF;
}

void Input::CheckInputState( const uint8_t iKey, GLFWwindow* pWindow )
{
	uint8_t iInputId = 0;
	switch( iKey )
	{
		case 0x0:
			iInputId = GLFW_KEY_0;
			break;
		case 0x1:
			iInputId = GLFW_KEY_1;
			break;
		case 0x2:
			iInputId = GLFW_KEY_2;
			break;
		case 0x3:
			iInputId = GLFW_KEY_3;
			break;
		case 0x4:
			iInputId = GLFW_KEY_4;
			break;
		case 0x5:
			iInputId = GLFW_KEY_5;
			break;
		case 0x6:
			iInputId = GLFW_KEY_6;
			break;
		case 0x7:
			iInputId = GLFW_KEY_7;
			break;
		case 0x8:
			iInputId = GLFW_KEY_8;
			break;
		case 0x9:
			iInputId = GLFW_KEY_9;
			break;
		case 0xA:
			iInputId = GLFW_KEY_A;
			break;
		case 0xB:
			iInputId = GLFW_KEY_B;
			break;
		case 0xC:
			iInputId = GLFW_KEY_C;
			break;
		case 0xD:
			iInputId = GLFW_KEY_D;
			break;
		case 0xE:
			iInputId = GLFW_KEY_E;
			break;
		case 0xF:
			iInputId = GLFW_KEY_F;
			break;
		default:
			std::cerr << "INPUT::KEY_NOT_IMPLEMENTED" << std::endl;
			break;
	}

	m_aInputs[ iKey ] = glfwGetKey( pWindow,iInputId ) == GLFW_PRESS;
}
