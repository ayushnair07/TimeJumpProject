#version 330 core
layout(location=0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 uView;
uniform mat4 uProj;

void main() {
    TexCoords = aPos;
    // remove translation from view matrix
    mat4 rotView = mat4(mat3(uView));
    gl_Position = uProj * rotView * vec4(aPos, 1.0);
}
