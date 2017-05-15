uniform sampler2D tex;

varying vec4 position;
varying vec2 texCoord;

#define X (1.0/1280.0*0.5)
#define Y (1.0/720.0*0.5)

void main()
{
	vec4 s;
	s  = texture2D(tex, texCoord + vec2(-X, -Y));
	s += texture2D(tex, texCoord + vec2(+X, -Y));
	s += texture2D(tex, texCoord + vec2(-X, +Y));
	s += texture2D(tex, texCoord + vec2(+X, +Y));

	gl_FragColor = s * 0.25;
}

