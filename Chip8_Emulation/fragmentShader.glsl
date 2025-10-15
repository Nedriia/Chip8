#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D oTexture;

void main()
{
	//FragColor = vec4( oColor, 1.0f );
	float v = texture( oTexture, TexCoord ).r;
	FragColor = vec4( v, v, v, 1.0 );/* * vec4( oColor, 1.0f )*/;
}
