#version 330 core

in vec2 vUv;
out vec4 FragColor;

uniform sampler2D u_sceneTex;
uniform float u_exposure;
uniform float u_contrast;
uniform float u_saturation;
uniform float u_gamma;
uniform float u_chromaticAberration;
uniform float u_filmGrain;
uniform float u_scanlineIntensity;
uniform float u_time;

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 toneMapReinhard(vec3 c) {
    return c / (1.0 + c);
}

void main() {
    vec2 center = vec2(0.5);
    vec2 dir = vUv - center;
    float dist = length(dir);

    vec3 color = texture(u_sceneTex, vUv).rgb;
    if (u_chromaticAberration > 0.0) {
        vec2 ca = dir * dist * u_chromaticAberration;
        float r = texture(u_sceneTex, vUv + ca).r;
        float g = texture(u_sceneTex, vUv).g;
        float b = texture(u_sceneTex, vUv - ca).b;
        color = vec3(r, g, b);
    }

    color *= u_exposure;
    color = toneMapReinhard(color);
    color = (color - 0.5) * u_contrast + 0.5;
    float luma = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(vec3(luma), color, u_saturation);

    float grain = hash12(vUv + vec2(u_time, u_time * 0.37)) - 0.5;
    color += grain * u_filmGrain;

    float scan = step(0.5, fract(gl_FragCoord.y * 0.5));
    color *= 1.0 - u_scanlineIntensity * scan;

    color = pow(max(color, vec3(0.0)), vec3(1.0 / max(u_gamma, 0.001)));
    FragColor = vec4(color, 1.0);
}
