uniform sampler2D tex;

varying vec4 position;
varying vec2 texCoord;

void main()
{
	gl_FragColor = texture2D(tex, vec2(texCoord.x, -texCoord.y));
}


