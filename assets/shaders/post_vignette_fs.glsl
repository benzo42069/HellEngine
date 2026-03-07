#version 330 core

in vec2 vUv;
out vec4 FragColor;

uniform sampler2D u_sceneTex;
uniform float u_vignetteIntensity;
uniform float u_vignetteRoundness;

void main() {
    vec3 color = texture(u_sceneTex, vUv).rgb;
    vec2 p = abs(vUv - vec2(0.5)) * 2.0;
    float roundness = max(u_vignetteRoundness, 0.05);
    float d = pow(pow(p.x, roundness) + pow(p.y, roundness), 1.0 / roundness);
    float vig = smoothstep(0.45, 1.0, d);
    color *= 1.0 - (u_vignetteIntensity * vig);
    FragColor = vec4(color, 1.0);
}
