#version 400
 
in vec3 direction;
 
uniform samplerCube skybox;

out vec4 frag_colour;
 
void main()
{    
	frag_colour = texture(skybox, direction);
}