#pragma once
#include <cstdint>
#include "Display.h"
#include <map>

struct GLFWwindow;
class alignas ( 16 ) Input
{

public:

	static Input* GetInstance()
	{
		if( m_pSingleton == nullptr )
			m_pSingleton = new Input;
		return m_pSingleton;
	}

	void ProcessInput( bool& quit );
	void DestroyInputManager();
	void InitInputFromDatabase( std::map<std::string,int>& aKeys );
	void InitInputDefault();

	uint8_t GetKeyState( uint8_t iIndex ) const;
	uint8_t	IsAnyKeyPress() const;

private:

	Input();
	~Input();

	void	CheckInputState( const uint8_t iKey,GLFWwindow* pWindow );
	static Input* m_pSingleton;
	std::map< uint8_t, int > m_aKeyMap;

	uint8_t m_aInputs[ 0x10 ];
	bool m_bEnglishLayout;
	bool m_bOverride;
};