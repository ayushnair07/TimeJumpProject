#version 330 core
out vec4 FragColor;

uniform vec3 uSunColor;

void main()
{
    // Just output the sun's color, fully bright
    FragColor = vec4(uSunColor, 1.0);
}