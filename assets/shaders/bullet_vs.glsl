#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUv;
layout (location = 2) in float aPaletteRow;
layout (location = 3) in float aAge;
layout (location = 4) in float aInstanceId;

uniform mat4 uViewProj;
uniform float uTime;
uniform float uAnimSpeed;
uniform vec2 uPerInstanceOffset;
uniform int uAnimMode;

out vec2 vUv;
out float vPaletteRow;
out float vAge;
out float vAnimPhase;

void main() {
    vec2 pos = aPos + (uPerInstanceOffset * fract(aInstanceId * 0.6180339));
    gl_Position = uViewProj * vec4(pos, 0.0, 1.0);
    vUv = aUv;
    vPaletteRow = aPaletteRow;
    vAge = aAge;

    float phase = (uTime + aAge) * uAnimSpeed;
    if (uAnimMode == 1) {
        phase = sin(phase);
    }
    vAnimPhase = phase;
}
