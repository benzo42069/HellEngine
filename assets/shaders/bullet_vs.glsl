#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUv;
layout (location = 2) in vec4 aColor;

uniform mat4 uViewProj;
uniform float uTime;
uniform float uAnimSpeed;
uniform vec2 uPerInstanceOffset;
uniform int uAnimMode;

out vec2 vUv;
out vec4 vColor;
out float vAnimPhase;

void main() {
    vec2 pos = aPos + uPerInstanceOffset;
    gl_Position = uViewProj * vec4(pos, 0.0, 1.0);
    vUv = aUv;
    vColor = aColor;
    float phase = uTime * uAnimSpeed;
    if (uAnimMode == 1) {
        phase = sin(phase);
    }
    vAnimPhase = phase;
}
