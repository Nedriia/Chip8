#pragma once
#include <deque>
#include "string"
#include <sstream>

class Chip8_Debugger
{
public:
	void Init( GLFWwindow* mainWindow );
	void Update();
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
	static void ConvertUintToString( std::string& sString,const uint16_t& num,const uint16_t& num2 = 0xFFF,const uint16_t& num3 = 0xFFF )
	{
		sString += std::to_string( num );
		if( num2 != 0xFFF )
			sString += ", " + std::to_string( num2 );
		if( num3 != 0xFFF )
			sString += ", " + std::to_string( num3 );
	}

	void AddEntryOpcode( const char* pOpcode );

private:
	static Chip8_Debugger* singleton;
	Chip8_Debugger();
	std::deque<std::string> opcodeHistory;
	bool	m_bFollowPc;

	GLFWwindow* window;
};

