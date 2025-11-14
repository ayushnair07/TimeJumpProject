#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 viewPos;
uniform samplerCube skybox;
uniform float uDayFactor;

// PBR uniforms from main.cpp
uniform vec3 baseColor;
uniform float metal;
uniform float roughness;

void main()
{
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    // Calculate reflection vector
    vec3 R = reflect(-V, N); 


    vec3 reflectionColor = texture(skybox, R).rgb;


    float day = clamp(uDayFactor, 0.0, 1.0);
    vec3 nightTint = vec3(0.01, 0.02, 0.05); // The same dark blue
    float mixFactor = pow(day, 0.5);
    

    vec3 finalReflection = mix(nightTint, reflectionColor, mixFactor);


    vec3 finalColor = mix(baseColor, finalReflection, metal);
    

    float fresnel = 1.0 - max(dot(N, V), 0.0);
    finalColor = mix(finalColor, finalReflection, fresnel * (1.0 - roughness));

    // Gamma correction
    finalColor = pow(finalColor, vec3(1.0/2.2));

    FragColor = vec4(finalColor, 1.0);
}