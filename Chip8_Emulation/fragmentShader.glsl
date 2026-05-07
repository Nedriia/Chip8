#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform usampler2D oTexture;
uniform vec3 bgColor;
uniform vec3 fontColor;

uniform int Width;
uniform int Height;

void main()
{
	vec2 oCoords = vec2( int( TexCoord.x * Width ), int ( TexCoord.y * Height ) );	//Convert texture ratio to coords
	int iBlock = int( oCoords.x ) / 32;												//Which block are we, texture sent as X block of 32 bits ( 2 for 64res )
	uvec4 texel = texelFetch( oTexture, ivec2( iBlock, oCoords.y ), 0 );			//Extract memory block
	uint iIndex = uint ( oCoords.x ) % 32u;											//Look for the pixel
	uint v = ( texel.r >> iIndex ) & 1u;

	vec3 color = mix( bgColor, fontColor, v );
	FragColor = vec4( color, 1.0 );
}
