#version 330 core
in vec3 TexCoords;
out vec4 FragColor;

uniform samplerCube skybox;
uniform float uDayFactor; // 0 = night, 1 = day
uniform sampler2D uStars; // optional: star texture (use white stars on black), bind unit 1; if not provided just skip

vec3 tonemap(vec3 x) {
    // simple ACES-like curve
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    vec3 color = texture(skybox, TexCoords).rgb;

    // basic day/night tint: reduce brightness at night and add bluish tint
    float day = clamp(uDayFactor, 0.0, 1.0);      // 0..1
    float night = 1.0 - day;

    // day: keep color, night: darken and tint slightly bluish
    vec3 nightTint = vec3(0.02, 0.05, 0.15);
    vec3 mixed = color * (0.6*day + 0.1) + nightTint * (0.9*night);

    // small contrast boost for day
    mixed = mix(mixed * 1.1, mixed, 0.0);

    // add stars at night (optional): sample a 2D star texture in screen-space direction
    vec3 starColor = vec3(0.0);
    if (night > 0.05) {
        // approximate spherical -> lat/long mapping of TexCoords for 2D stars lookup
        float u = 0.5 + atan(TexCoords.z, TexCoords.x) / (2.0 * 3.14159265);
        float v = 0.5 - asin(clamp(TexCoords.y, -1.0, 1.0)) / 3.14159265;
        vec3 stars = texture(uStars, vec2(u, v)).rgb; // bind a stars image to unit 1 (if available)
        starColor = stars * night * 1.5; // scale by nightness
    }

    vec3 outc = mix(mixed, tonemap(mixed), day); // mild tonemapping in day
    outc += starColor;

    // gamma
    outc = pow(outc, vec3(1.0 / 2.2));
    FragColor = vec4(outc, 1.0);
}
