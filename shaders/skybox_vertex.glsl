attribute vec3 position;

varying vec3 v_TexCoords;

uniform mat4 u_MVMatrix;
uniform mat4 u_PMatrix;

void main() {
   v_TexCoords = vec3(-position.x, position.y, -position.z);

   mat4 rotationMatrix = mat4(mat3(u_MVMatrix));

   gl_Position = u_PMatrix * rotationMatrix * vec4(position, 1.0);

}