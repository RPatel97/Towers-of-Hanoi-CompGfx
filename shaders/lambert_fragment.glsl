#version 330

uniform vec3 u_color; // RGB
uniform vec4 u_lightPosDir; // W component is 'boolean'

in vec3 surfaceNormal;
in vec3 worldPosition;
in vec2 textureCoordinates;

uniform int u_textured;
uniform sampler2D u_texture;

void main() {

    vec3 normal = normalize(surfaceNormal);
    vec3 lightDirection = vec3(0.0f);

	if(u_textured == 0){
		if (u_lightPosDir.w == 1.0f) //Point light 
		{
			// Do the math here
			lightDirection = normalize(worldPosition - u_lightPosDir.xyz);

		} else //Directional
		{
			lightDirection = normalize(u_lightPosDir.xyz);
		}

		 // LAMBERT 
		float diffuse = max(0.0f, dot(normal, lightDirection));

		gl_FragColor = vec4(u_color * diffuse, 1.0);
		//gl_FragColor = vec4(u_color, 1.0);
	}

	if (u_textured == 1){
		if (u_lightPosDir.w == 1.0f) //Point light 
			{
				// Do the math here
				lightDirection = normalize(worldPosition - u_lightPosDir.xyz);

			} else //Directional
			{
				lightDirection = normalize(u_lightPosDir.xyz);
			}

		vec4 texture = texture2D(u_texture, textureCoordinates);
		float diffuse = max(0.0f, dot(normal, lightDirection));
		gl_FragColor = vec4(texture.rgb * diffuse, 1.0);
	}

}
