#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
class Display
{

public:
	Display();
	int Init();
	int CreateWindowChip();
	void DestroyWindow();
	void Update();

private:
	GLFWwindow* window;
};
