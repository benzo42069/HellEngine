#version 330 core

in vec2 vUv;
in vec4 vColor;

uniform sampler2D uSpriteAtlas;
uniform sampler2D uPaletteRamp;
uniform float uEmissiveBoost;
uniform float uTime;

out vec4 FragColor;

void main() {
    float sdf = texture(uSpriteAtlas, vUv).r;
    float alphaMask = smoothstep(0.12, 0.9, sdf);
    vec3 lit = vColor.rgb * max(uEmissiveBoost, 0.0);
    FragColor = vec4(lit, vColor.a * alphaMask);
}
