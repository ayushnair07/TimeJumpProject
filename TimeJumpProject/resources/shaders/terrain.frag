#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D uTex;          // albedo/texture
uniform vec3 lightPos;           // (existing main directional/sun used earlier if any)
uniform vec3 viewPos;
uniform vec3 lightColor;         // sun color
uniform vec3 ambientColor;       // ambient

// ----- Spotlight struct -----
struct SpotLight {
    vec3 position;
    vec3 direction;      // normalized
    float cutOff;        // cos(innerAngle)
    float outerCutOff;   // cos(outerAngle)
    vec3 color;
    float intensity;     // scalar multiplier
    // attenuation coefficients
    float constant;
    float linear;
    float quadratic;
    bool enabled;
};

uniform SpotLight spot;   // single spotlight forward-declared

// simple Blinn-Phong calc with spotlight factor
vec3 CalcSpotLight(SpotLight s, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo) {
    if (!s.enabled) return vec3(0.0);

    vec3 lightDir = normalize(s.position - fragPos); // from fragment -> light
    // compute attenuation
    float distance = length(s.position - fragPos);
    float attenuation = 1.0 / (s.constant + s.linear * distance + s.quadratic * (distance * distance));

    // spotlight intensity (smooth step between inner/outer)
    float theta = dot(lightDir, normalize(-s.direction)); // note: direction is "where it points"
    float epsilon = (s.cutOff - s.outerCutOff);
    float spotFactor = clamp((theta - s.outerCutOff) / max(epsilon, 0.0001), 0.0, 1.0);

    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * s.color * albedo;

    // specular (Blinn)
    vec3 halfVec = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfVec), 0.0), 32.0);
    vec3 specular = spec * s.color * 0.3;

    vec3 result = (diffuse + specular) * s.intensity * spotFactor * attenuation;
    return result;
}

void main() {
    vec3 albedo = texture(uTex, TexCoord).rgb;

    // ambient + directional(sun) from your previous code
    vec3 ambient = ambientColor * albedo;

    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * lightColor * albedo;

    vec3 V = normalize(viewPos - FragPos);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0);
    vec3 specular = vec3(0.3) * spec * lightColor;

    // spotlight contribution
    vec3 spotContrib = CalcSpotLight(spot, N, FragPos, V, albedo);

    vec3 color = ambient + diffuse + specular + spotContrib;
    FragColor = vec4(color, 1.0);
}
