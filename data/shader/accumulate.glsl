uniform sampler2D tex;
uniform sampler2D old;
uniform float flip;

varying vec4 position;
varying vec2 texCoord;

void main()
{
	vec4 a = texture2D(old, vec2(texCoord.x, 1.0 - texCoord.y));
	vec4 b = texture2D(tex, texCoord);
	gl_FragColor = mix(a, b, b.a * 0.1);
}



