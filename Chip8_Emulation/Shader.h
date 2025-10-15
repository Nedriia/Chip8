#pragma once

#include <glad/glad.h>

class Shader
{
public:
	Shader();
	Shader( const char* VertexPath, const char* FragmentPath );
	void Use();
	void Delete();

	unsigned int ID;
};