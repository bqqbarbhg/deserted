uniform sampler2D tex;
uniform float flip;

varying vec4 position;
varying vec2 texCoord;

void main()
{
	gl_FragColor = texture2D(tex, vec2(texCoord.x, flip > 0.5 ? 1.0 - texCoord.y : texCoord.y));
}


