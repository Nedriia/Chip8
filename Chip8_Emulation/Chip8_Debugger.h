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
		if( singleton == nullptr )
			singleton = new Chip8_Debugger;
		return singleton;
	}
	static void ConvertHexToString( std::string& sString,const uint16_t& num,const uint16_t& num2 = 0,const uint16_t& num3 = 0 )
	{
		std::stringstream ss;
		ss << "0x" << std::hex << num;
		sString += ss.str();

		if( num2 != 0 )
			ss << ", 0x" << std::hex << num2;

		if( num3 != 0 )
			ss << ", 0x" << std::hex << num3;
	}

private:
	template< typename T >
	void FormatDebugData( std::string sText,const char* sFormat, T oData );

	static Chip8_Debugger*		singleton;

	GLFWwindow*					window;
	const Chip8*				m_pCPU;
};