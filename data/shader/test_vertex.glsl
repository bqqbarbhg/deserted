
attribute vec4 a_position;
attribute vec2 a_texCoord;

varying vec4 position;
varying vec2 texCoord;

void main()
{
	position = a_position;
	texCoord = a_texCoord;
	gl_Position = a_position;
}


