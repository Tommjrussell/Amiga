#version 330 core

in vec2 fragCoord;
out vec3 fragColor;

uniform vec2 winSize;
uniform vec2 screenSize;
uniform sampler2D myTextureSampler;
uniform float magnification;

void main()
{
	vec2 uv = fragCoord / vec2(1024, 512);

	fragColor = texture( myTextureSampler, uv).rgb;
}