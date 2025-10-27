#pragma once

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

private:
	static Chip8_Debugger* singleton;
	Chip8_Debugger();

	GLFWwindow* window;
};

