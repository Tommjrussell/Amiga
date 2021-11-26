#version 330 core

layout(location = 0) in vec3 vert;
layout(location = 1) in vec2 vertexUV;

uniform vec2 winSize;

out vec2 fragCoord;

void main()
{
	const float targetRatio = 4.0/3.0;

	float winRatio = (winSize.x / winSize.y);

	vec2 fix = vec2(1.0,1.0);

	if (winRatio > targetRatio)
	{
		fix.x = fix.x * (targetRatio / winRatio);
	}
	else
	{
		fix.y = fix.y * (winRatio / targetRatio);
	}

    gl_Position = vec4(vert.xy * fix, 0, 1);
	fragCoord = vertexUV;
}