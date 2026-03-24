#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D oTexture;
vec3 bgColor = vec3( 0.67,0.27,0.0 );
vec3 fontColor = vec3( 1.0,0.67,0.0 );

void main()
{
	float v = texture( oTexture, TexCoord ).r;
	vec3 color = mix( bgColor, fontColor, v );
	FragColor = vec4( color, 1.0 );
}
