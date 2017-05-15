uniform sampler2D tex;
uniform float flip;

varying vec4 position;
varying vec2 texCoord;

float cmpColor(vec3 a, vec3 b)
{
	vec3 d = a - b;
	float mono = d.r + d.g + d.b;

	return length(d);
}

void main()
{
	vec4 value;
	float num;

	vec4 self = texture2D(tex, texCoord);

	for (float x = -2.0; x <= 2.001; x += 1.0)
	{
		for (float y = -2.0; y <= 2.001; y += 1.0)
		{
			vec4 sample = texture2D(tex, texCoord + vec2(x, y) * 0.001);
			if (cmpColor(self, sample) < 0.05)
			{
				value += sample;
				num += 1.0;
			}
		}
	}

	gl_FragColor = value / num;
}

