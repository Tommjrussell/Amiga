#version 330 core

layout(location = 0) in vec3 vert;
layout(location = 1) in vec2 vertexUV;

uniform vec2 winSize;

out vec2 fragCoord;

void main()
{
    gl_Position = vec4(vert, 1);
	fragCoord = vertexUV;
}