#version 330
uniform mat4 u_MVP;

attribute vec4 position;
attribute vec2 textureCoords;
attribute vec3 normal;

out vec3 surfaceNormal;
out vec3 worldPosition;
out vec2 textureCoordinates;

void main() {
    gl_Position = u_MVP * position;

    worldPosition = gl_Position.xyz;
    surfaceNormal = normal;
	textureCoordinates = textureCoords;
    //textureCoordinates.y = 1.0f - textureCoordinates.y;
}
