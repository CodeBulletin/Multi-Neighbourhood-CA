#version 450
//#pragma optionNV unroll all

out vec4 frag;

uniform sampler2D tex;
uniform float Width;
uniform float Height;

struct Rule {
	ivec2 range;
	vec2 aliveRange;
	vec2 deadRange;
};

uniform float dt = 0.048;
const int numSensors = <num>;
uniform Rule rules[numSensors];

vec2 wrapAround(vec2 coord) {
	return mod(coord + vec2(1, 1), vec2(1, 1));
}

float getNeighbourValue(vec2 coord, vec2 neighbour) {
	vec2 mapSize = vec2(Width, Height);
	return texture2D(tex, wrapAround((coord + neighbour) / mapSize)).x;
}

float getNeighbourSums(vec2 coord, ivec2 range) {
	float cellCount = 0;
	float livingValueSum = 0;

	int radiusMin = range.x;
	int radiusMax = range.y;

	for (int offsetX = -radiusMax; offsetX <= radiusMax; offsetX += 1) {
		for (int offsetY = -radiusMax; offsetY <= radiusMax; offsetY += 1) {

			vec2 offset = vec2(offsetX, offsetY);
			float sqrDst = dot(offset, offset);
			int dst = int(round(sqrt(sqrDst)));

			if (dst <= radiusMax && dst > radiusMin) {
				float neighbourValue = getNeighbourValue(coord, offset).x;
				cellCount += 1;
				livingValueSum += neighbourValue;
			}
		}
	} 
	return livingValueSum / cellCount;
}

void main() {
	const int numPotentialStates = numSensors / 2;

	vec4 prevValues = texture2D(tex, gl_FragCoord.xy / vec2(Width, Height));
	float prevState = prevValues.x;

	float sensorData [numSensors];

	for (int i = 0; i < numSensors; i ++) {
		sensorData[i] = getNeighbourSums(gl_FragCoord.xy, rules[i].range);
	}

	float potentialStates[numPotentialStates];

	for (int i = 0; i < numPotentialStates; i ++) {
		potentialStates[i] = prevState;
	}

	for (int i = 0; i < numSensors; i ++) {
		Rule rule = rules[i];
		float sensorValue = sensorData[i];
		if (sensorValue >= rule.aliveRange.x && sensorValue <= rule.aliveRange.y) {
			potentialStates[i / 2] += dt;
		}
		if (sensorValue >= rule.deadRange.x && sensorValue <= rule.deadRange.y) {
			potentialStates[i / 2] -= dt;
		}
	}

	for (int i = 0; i < numPotentialStates; i ++) {
		float avgValInSensors = (sensorData[i * 2] + sensorData[i * 2 + 1]) * 0.5;
		potentialStates[i] = (potentialStates[i] + avgValInSensors * dt) / (1 + dt);
	}

	float maxDelta = 0;
	int selectedStateIndex = 0;

	for (int i = 0; i < numPotentialStates; i++ ) {
		float delta = abs(prevState - potentialStates[i]);
		if (delta > maxDelta) {
			maxDelta = delta;
			selectedStateIndex = i;
		}
	}

	float nextState = clamp(potentialStates[selectedStateIndex], 0, 1);
	//vec4 col = texture2D(tex, gl_FragCoord.xy / vec2(Width, Height));
	//col.y = getState(gl_FragCoord.xy, col.x, rules[0]);
	frag = vec4(prevState, nextState, 1.0f, 1.0f);
}