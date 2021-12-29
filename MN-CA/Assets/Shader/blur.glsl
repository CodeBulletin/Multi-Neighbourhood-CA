#version 450

uniform sampler2D tex;
uniform vec2 rad; // step size
uniform float Width, Height;
out vec4 frag;

void main() {
	vec2 tcoord = gl_FragCoord.xy;
	vec2 coord = tcoord / vec2(Width, Height);

	vec2 dir = rad / vec2(Width, Height);

	vec4 color = vec4(0.0);
	color += texture2D(tex, coord - 4.0 * dir) * 0.0162162162;
	color += texture2D(tex, coord - 3.0 * dir) * 0.0540540541;
	color += texture2D(tex, coord - 2.0 * dir) * 0.1216216216;
	color += texture2D(tex, coord - dir) * 0.1945945946;
	color += texture2D(tex, coord) * 0.2270270270;
	color += texture2D(tex, coord + dir) * 0.1945945946;
	color += texture2D(tex, coord + 2.0 * dir) * 0.1216216216;
	color += texture2D(tex, coord + 3.0 * dir) * 0.0540540541;
	color += texture2D(tex, coord + 4.0 * dir) * 0.0162162162;

	frag = color;
}