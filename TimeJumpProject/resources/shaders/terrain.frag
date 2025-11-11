#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D uTex;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 ambientColor;

void main(){
    vec3 albedo = texture(uTex, TexCoord).rgb;

    // ambient
    vec3 ambient = ambientColor * albedo;

    // diffuse
    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);
    float diff = max(dot(N,L), 0.0);
    vec3 diffuse = diff * lightColor * albedo;

    // specular (Blinn-Phong)
    vec3 V = normalize(viewPos - FragPos);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N,H), 0.0), 32.0);
    vec3 specular = vec3(0.3) * spec * lightColor;

    vec3 color = ambient + diffuse + specular;
    FragColor = vec4(color, 1.0);
}
