#version 330 core

in vec2 vUv;
in vec4 vColor;
in float vAnimPhase;

uniform sampler2D uSpriteAtlas;
uniform sampler2D uPaletteRamp;
uniform float uEmissiveBoost;
uniform int uAnimMode;

out vec4 FragColor;

void main() {
    float sdf = texture(uSpriteAtlas, vUv).r;
    float alpha = smoothstep(0.12, 0.9, sdf);

    float rampCoord = clamp(sdf + (uAnimMode == 2 ? 0.1 * sin(vAnimPhase) : 0.0), 0.0, 1.0);
    vec3 rampColor = texture(uPaletteRamp, vec2(rampCoord, 0.5)).rgb;

    vec3 lit = vColor.rgb * mix(vec3(1.0), rampColor, 0.5) * max(uEmissiveBoost, 0.0);
    FragColor = vec4(lit, vColor.a * alpha);
}
