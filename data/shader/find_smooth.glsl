uniform sampler2D tex;

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
	vec2 uv = texCoord;
	vec3 self = texture2D(tex, uv).rgb;

	float totalDiff = 0.0;

	for (float sx = -1.0; sx <= 1.001; sx += 1.0)
	{
		for (float sy = -1.0; sy <= 1.001; sy += 1.0)
		{
			vec3 ref = texture2D(tex, uv + vec2(sx, sy) * 0.003).rgb;
			totalDiff += cmpColor(self, ref);
		}
	}

	totalDiff *= 1.0;
	gl_FragColor = vec4(self, 1.0 - totalDiff);
}

