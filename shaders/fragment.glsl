#version 330

uniform vec3 u_DiffuseColour;

in vec2 TexCoords;
in vec3 Normal;

void main() {
    gl_FragColor = vec4(u_DiffuseColour, 1.0);
}
