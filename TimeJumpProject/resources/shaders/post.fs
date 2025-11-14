#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScene;  // rendered scene texture
uniform float uJump;       // 0..1 (0 normal, 1 full effect)
uniform float uTime;       // for grain animation

const vec3 LUMA = vec3(0.2126, 0.7152, 0.0722);

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec3 desat(vec3 c, float amount) {
    float l = dot(c, LUMA);
    return mix(c, vec3(l), amount);
}

void main() {
    vec4 sc = texture(uScene, vUV);
    vec3 color = sc.rgb;

    // vignette
    vec2 p = vUV - 0.5;
    float d = length(p);
    float vign = smoothstep(0.85, 0.45, d); // 0 center -> 1 edges
    vign = clamp(vign, 0.0, 1.0);

    // desaturation & tint
    float desatAmount = mix(0.0, 0.65, uJump);
    color = desat(color, desatAmount);
    vec3 sandTint = vec3(0.95, 0.86, 0.64);
    vec3 nightTint = vec3(0.05, 0.08, 0.18);
    // choose the “destination” look; adjust mixing factor to taste
    vec3 tint = mix(sandTint, nightTint, 1.0); // using desert->night? keep one
    color = mix(color, color * tint, uJump * 0.6);

    // darken
    color *= mix(1.0, 0.45, uJump);

    // vignette application
    color *= mix(vec3(1.0), vec3(vign), uJump * 0.9);

    // grain
    float g = rand(vUV * (uTime * 0.1 + 1.0));
    color += (g * 2.0 - 1.0) * (0.03 * uJump);

    // tiny contrast/gamma tweak
    color = pow(color, vec3(1.0 / mix(1.0, 1.08, uJump)));

    FragColor = vec4(clamp(color, 0.0, 1.0), sc.a);
}
