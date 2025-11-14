#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
// instance matrix
layout(location=3) in vec4 iMat0;
layout(location=4) in vec4 iMat1;
layout(location=5) in vec4 iMat2;
layout(location=6) in vec4 iMat3;

uniform mat4 uView;
uniform mat4 uProj;

out vec2 vUV;
out vec3 vNormal;
out vec3 vWorldPos;

void main() {
    mat4 inst = mat4(iMat0, iMat1, iMat2, iMat3);
    vec4 worldPos = inst * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(inst) * aNormal; // simple normal transform
    vUV = aUV;
    gl_Position = uProj * uView * worldPos;
}
