#pragma once
#include <cstdint>
#include <chrono>

struct GLFWwindow;
class Input
{
public:
	Input();
	~Input(){};

	static Input* GetInstance()
	{
		if( m_pSingleton == nullptr )
			m_pSingleton = new Input;
		return m_pSingleton;
	}
	void ProcessInput( const std::chrono::steady_clock::time_point& time,bool& quit );

	uint8_t GetKeyState( uint8_t iIndex ) const;
	uint8_t	IsAnyKeyPress() const;

private:
	void	CheckInputState( const uint8_t iKey,GLFWwindow* pWindow );
	static Input* m_pSingleton;

	uint8_t m_aInputs[ 0x10 ];
	std::chrono::steady_clock::time_point m_iLastTimeUpdate;
};