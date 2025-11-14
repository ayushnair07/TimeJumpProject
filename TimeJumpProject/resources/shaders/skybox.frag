#version 330 core
in vec3 TexCoords;
out vec4 FragColor;

uniform samplerCube skybox;
uniform float uDayFactor; // 0 = night, 1 = day
uniform sampler2D uStars; 

vec3 tonemap(vec3 x) {
    // simple ACES-like curve
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}
// -------------------------------------------

void main() {
    vec3 dayColor = texture(skybox, TexCoords).rgb;

    float day = clamp(uDayFactor, 0.0, 1.0);
    float night = 1.0 - day;

    // --- REVISED LOGIC ---
    // At night, we want a dark tint. In day, we want the skybox color.
    // We will mix between them.
    vec3 nightTint = vec3(0.01, 0.02, 0.05); // Made this darker
    
    // Use a power curve (pow(day, 0.5)) so the sun
    // and sky brighten up more quickly at "sunrise".
    float mixFactor = pow(day, 0.5); 
    vec3 mixed = mix(nightTint, dayColor, mixFactor);

    // --- End of revised logic ---

    vec3 starColor = vec3(0.0);
    if (night > 0.05) {
        // ... (same star logic as before)
        float u = 0.5 + atan(TexCoords.z, TexCoords.x) / (2.0 * 3.14159265);
        float v = 0.5 - asin(clamp(TexCoords.y, -1.0, 1.0)) / 3.14159265;
        vec3 stars = texture(uStars, vec2(u, v)).rgb; 
        starColor = stars * night * 1.5; 
    }

    vec3 outc = mixed; // Use the new 'mixed' color
    outc += starColor; // Add stars on top

    // gamma
    outc = pow(outc, vec3(1.0 / 2.2));
    FragColor = vec4(outc, 1.0);
}