#version 330 core

in vec2 vUv;
in float vPaletteRow;
in float vAge;
in float vAnimPhase;

uniform sampler2D uSpriteAtlas;
uniform sampler2D uPaletteRamp;
uniform float uEmissiveBoost;
uniform int uAnimMode;

out vec4 FragColor;

void main() {
    float sdf = texture(uSpriteAtlas, vUv).r;
    float alpha = smoothstep(0.12, 0.9, sdf);

    float rampCoord = sdf;
    if (uAnimMode == 2) {
        rampCoord = clamp(sdf + 0.1 * sin(vAnimPhase), 0.0, 1.0);
    }
    vec3 rampColor = texture(uPaletteRamp, vec2(rampCoord, vPaletteRow)).rgb;

    float agePulse = 0.9 + 0.1 * sin(vAge * 2.0 + vAnimPhase);
    vec3 lit = rampColor * max(uEmissiveBoost, 0.0) * agePulse;
    FragColor = vec4(lit, alpha);
}
