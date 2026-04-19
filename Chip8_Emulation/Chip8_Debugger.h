#pragma once
#include "string"
#include <sstream>
#include <chrono>

class Chip8;
class Chip8_Debugger
{
public:
	Chip8_Debugger();

	void Init( GLFWwindow* mainWindow,const Chip8* pCPU );
	void Update( const std::chrono::microseconds& time );
	void Render();

	static Chip8_Debugger* GetInstance()
	{
		if( m_pSingleton == nullptr )
			m_pSingleton = new Chip8_Debugger;
		return m_pSingleton;
	}

private:
	template< typename T >
	void FormatDebugData( std::string sText,const char* sFormat, const T& oData, int& iIndexSelectable, int& iIndexPosition );

	static Chip8_Debugger*		m_pSingleton;

	GLFWwindow*					m_pWindow;
	const Chip8*				m_pCPU;
	int							m_iCycleIndex;

	int							m_iRegisterSelected;
	int							m_iMemorySelected;
	int							m_iStackSelected;
};