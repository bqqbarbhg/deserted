uniform sampler2D tex;
uniform float pass;
uniform float time;
uniform vec2 mouse;

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
	float cellSz = 1.0 / 16.0;
	vec2 uv = texCoord;
	float step = 1.0 / 5.0;

	vec4 selfFull = texture2D(tex, uv);
	vec3 self = selfFull;

	vec3 colTotal = vec3(0.0);
	float colNum = 0.0;
	vec2 xyTotal = vec2(0.0);

	float cutoff = mouse.x * 0.1;

	for (float stepX = 0.0; stepX < 1.0001; stepX += step)
	{
		for (float stepY = 0.0; stepY < 1.0001; stepY += step)
		{
			vec2 cell = vec2(uv.x - mod(uv.x + time * 0.0, cellSz), uv.y - mod(uv.y + time * 0.0, cellSz));
			vec2 pos = cell + vec2(stepX, stepY) * cellSz;
			vec4 ref = texture2D(tex, pos);

			float d = cmpColor(self, ref.rgb);
			if (d < cutoff)
			{
				float f = 1.0 - clamp(d, 0.0, 1.0);
				colTotal += ref.rgb;
				colNum += 1.0;
				xyTotal += vec2(stepX, stepY);
			}
		}
	}

	if (colNum >= 2.5 && length(colTotal) > 0.5)
		gl_FragColor = vec4(colTotal / colNum, 1.0);
	else
		gl_FragColor = vec4(vec3(0.0), 0.0);
}


