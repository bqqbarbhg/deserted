
uniform sampler2D tex;
uniform sampler2D framebuffer;

varying vec4 position;
varying vec2 texCoord;

uniform vec2 mouse;

void main()
{
	gl_FragColor = texture2D(framebuffer, texCoord) * 0.9 + texture2D(tex, texCoord + mouse) * 0.1;
	// gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}

