#version 400

layout (location = 0) in vec3 vp;
 
uniform mat4 m;
uniform mat4 v;
uniform mat4 p;
 
out vec3 direction;
 
void main()
{
    direction = vp;
	gl_Position = p * v * vec4(vp, 1.0);
}