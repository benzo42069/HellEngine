#version 330 core

in vec2 vUv;
out vec4 FragColor;

uniform sampler2D u_sceneTex;
uniform sampler2D u_bloomTex;
uniform vec2 u_texelSize;
uniform float u_bloomThreshold;
uniform float u_bloomIntensity;
uniform int u_mode;

float luminance(vec3 c) {
    return dot(c, vec3(0.299, 0.587, 0.114));
}

void main() {
    if (u_mode == 0) {
        vec3 color = texture(u_sceneTex, vUv).rgb;
        float luma = luminance(color);
        float mask = smoothstep(u_bloomThreshold, u_bloomThreshold + 0.25, luma);
        FragColor = vec4(color * mask, 1.0);
        return;
    }

    if (u_mode == 1 || u_mode == 2) {
        vec2 offset = u_texelSize;
        vec3 sum = vec3(0.0);
        sum += texture(u_sceneTex, vUv + vec2(-offset.x, -offset.y)).rgb;
        sum += texture(u_sceneTex, vUv + vec2( offset.x, -offset.y)).rgb;
        sum += texture(u_sceneTex, vUv + vec2(-offset.x,  offset.y)).rgb;
        sum += texture(u_sceneTex, vUv + vec2( offset.x,  offset.y)).rgb;
        FragColor = vec4(sum * 0.25, 1.0);
        return;
    }

    vec3 sceneColor = texture(u_sceneTex, vUv).rgb;
    vec3 bloomColor = texture(u_bloomTex, vUv).rgb;
    FragColor = vec4(sceneColor + bloomColor * u_bloomIntensity, 1.0);
}
