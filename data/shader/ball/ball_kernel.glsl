
uniform sampler2D tex;

uniform float radius;

varying vec4 position;
varying vec2 texCoord;

#define IX (1.0/1280.0)
#define IY (1.0/720.0)

void main()
{
	float s;

	s += texture2D(tex, texCoord + vec2(radius * IX, 0.0)).x;
	s += texture2D(tex, texCoord - vec2(radius * IX, 0.0)).x;
	s += texture2D(tex, texCoord + vec2(0.0, radius * IY)).y;
	s += texture2D(tex, texCoord - vec2(0.0, radius * IY)).y;

	gl_FragColor = vec4(vec3(s - 2.0), 1.0);
}

