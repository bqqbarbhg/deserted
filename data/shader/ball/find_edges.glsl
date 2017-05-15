uniform sampler2D tex;

uniform float multiplier;

varying vec4 position;
varying vec2 texCoord;

void main()
{
	vec4 tex = texture2D(tex, texCoord);
	float lum = dot(tex.rgb, vec3(0.33));
	vec2 deriv = vec2(dFdx(lum), dFdy(lum));
	vec2 dd = abs(deriv) * multiplier;

	gl_FragColor = vec4(dd.x, dd.y, 0.0, 1.0);
}

