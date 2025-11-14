#version 330 core
layout(location=0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 uView;
uniform mat4 uProj;

void main() {
    TexCoords = aPos;
    
    // 1. Remove translation from view matrix
    mat4 rotView = mat4(mat3(uView)); 
    
    // 2. Calculate position
    vec4 pos = uProj * rotView * vec4(aPos, 1.0);
    
    // 3. THE FIX: Set z to w. 
    // When OpenGL performs perspective divide (z / w), the result will be 1.0 (maximum depth).
    gl_Position = pos.xyww; 
}