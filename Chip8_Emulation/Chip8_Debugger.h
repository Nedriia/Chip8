#pragma once
#include <deque>

class Chip8_Debugger
{
public:
	void Init( GLFWwindow* mainWindow );
	void Update();
	void Render();

	static Chip8_Debugger* GetInstance()
	{
		if (singleton == nullptr)
			singleton = new Chip8_Debugger;
		return singleton;
	}

	void AddEntryOpcode( const char* pOpcode );

private:
	static Chip8_Debugger* singleton;
	Chip8_Debugger();
	std::deque<const char*> opcodeHistory;

	GLFWwindow* window;
};

