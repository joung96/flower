#version 330 

uniform mat4 uProjMatrix; 
uniform mat4 uModelViewMatrix; 
uniform mat4 uNormalMatrix; 

layout (location = 0) in vec3 inPosition; 
layout (location = 1) in vec2 inCoord; 
layout (location = 2) in vec3 inNormal; 

out vec2 texCoord; 

smooth out vec3 vNormal; 

void main() 
{ 
   gl_Position = uProjMatrix*uModelViewMatrix
  *vec4(inPosition, 1.0); 
   texCoord = inCoord; 
   vec4 vRes = uNormalMatrix*vec4(inNormal, 0.0); 
   vNormal = vRes.xyz; 
}