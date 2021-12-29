#version 450

out vec4 frag;

uniform sampler2D tex;
uniform float Width;
uniform float Height;

void main() {
	float s = texture2D(tex, gl_FragCoord.xy / vec2(Width, Height)).x;
	float threshold = 0.05;
	s = (s - threshold) / (1 - threshold);
	frag = vec4(s, s, s, 1.0);
}
