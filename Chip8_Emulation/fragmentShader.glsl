#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform usampler2D oTexture;
vec3 bgColor = vec3( 0.67,0.27,0.0 );
vec3 fontColor = vec3( 1.0,0.67,0.0 );

void main()
{
	vec2 oCoords = vec2( int( TexCoord.x * 64 ), int ( TexCoord.y * 32 ) ); //Convert texture ratio to coords
	int iBlock = int( oCoords.x ) / 32;										//Which block are we, texture sent as 2 block of 32 bits
	uvec4 texel = texelFetch( oTexture, ivec2( iBlock, oCoords.y ), 0 );	//Extract memory block
	uint iIndex = uint ( oCoords.x ) % 32u;									//Look for the pixel
	uint v = ( texel.r >> iIndex ) & 1u;

	vec3 color = mix( bgColor, fontColor, v );
	FragColor = vec4( color, 1.0 );
}
