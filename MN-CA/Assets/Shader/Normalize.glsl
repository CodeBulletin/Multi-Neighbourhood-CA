#version 450

uniform sampler2D tex;
uniform float Width, Height;
uniform float min_val, max_val;

out vec4 frag;

float map(float v, float valmin, float valmax, float mapmin, float mapmax) {
	return mapmin + (mapmax - mapmin) * ((v - valmin) / (valmax - valmin));
}

void main() {
	float s = texture2D(tex, gl_FragCoord.xy / vec2(Width, Height)).g;

	s = map(s, min_val, max_val, 0, 1);

	frag = vec4(s, s, s, 1);
}