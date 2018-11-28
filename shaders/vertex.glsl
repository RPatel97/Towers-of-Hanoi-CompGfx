#version 330

layout(location = 0) in vec4 a_Position;
layout(location = 1) in vec2 a_TexCoords;
layout(location = 2) in vec3 a_Normal;

uniform mat4 u_MVP;

out vec2 TexCoords;
out vec3 Normal;

void main() {
	TexCoords = a_TexCoords;
	Normal = a_Normal;

	gl_Position = u_MVP * a_Position;
}