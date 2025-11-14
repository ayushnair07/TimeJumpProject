#version 330 core
in vec2 vUV;
in vec3 vNormal;
in vec3 vWorldPos;
out vec4 FragColor;

uniform sampler2D uTex;        // diffuse atlas / texture
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec3 viewPos;

void main(){
    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPos - vWorldPos);
    float diff = max(dot(N,L), 0.0);
    vec3 col = texture(uTex, vUV).rgb;
    // if texture has alpha and we want to discard fully transparent pixels:
    float a = texture(uTex, vUV).a;
    if (a < 0.5) discard;
    vec3 ambient = ambientColor * col;
    vec3 diffuse = diff * lightColor * col;
    FragColor = vec4(ambient + diffuse, 1.0);
}
