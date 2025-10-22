#pragma once

class Chip8_Debugger
{
public:
	Chip8_Debugger();
	void Init( GLFWwindow* mainWindow );
	void Update();
	void Render();

private:
	GLFWwindow* window;
};

