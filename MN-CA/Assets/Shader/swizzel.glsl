#version 450

uniform sampler2D tex;
uniform float Width, Height;
uniform vec4 c;

out vec4 frag;

float lerp(float t, float a, float b) {
	return a * (1 - t) + b * t;
}

vec4 lerp(float t, vec4 a, vec4 b) {
	return vec4(
		lerp(t, a.x, b.x),
		lerp(t, a.y, b.y),
		lerp(t, a.z, b.z),
		lerp(t, a.w, b.w)
	);
}

void main() {
	float s = texture2D(tex, gl_FragCoord.xy / vec2(Width, Height)).y;
	frag = lerp(s*s*s, vec4(0, 0, 0, 1), c);
}