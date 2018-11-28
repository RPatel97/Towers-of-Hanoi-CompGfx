varying vec3 v_TexCoords;

uniform samplerCube u_TextureSampler;

void main() {
   gl_FragColor = texture(u_TextureSampler, v_TexCoords);
}