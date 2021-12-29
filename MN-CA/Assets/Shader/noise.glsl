#version 450

out vec4 fragment;
uniform vec2 displacement;
uniform int Width, Height;

float hash(vec2 p) {
    vec3 p3 = vec3(p, 0);
	p3 = fract(p3 * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return fract((p3.xxy + p3.yxx) * p3.zyx).x;
}

float init(vec2 id) {
	float dstFromCentre = length(id.xy - vec2(Width, Height) * 0.5);
	return hash(id.xy + vec2(displacement * Width));
}

void main() {
	float val = init(gl_FragCoord.xy + displacement);
    fragment = vec4( vec3(val), 1.0 );
}