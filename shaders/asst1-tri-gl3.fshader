#version 150

uniform float uVertexScale;
uniform sampler2D uTex2;

in vec2 vTexCoord;
in vec2 vTemp;
in vec4 vColor;

out vec4 fragColor;

void main(void) {
  // vec4 color = vec4(vTemp.x, vTemp.y, 0.5, 1);
	
  // The texture(...) calls always return a vec4. Data comes out of a texture in RGBA format
  vec4 texColor0 = texture(uTex2, vTexCoord);


  // fragColor is a vec4. The components are interpreted as red, green, blue, and alpha
  vec4 a = vec4((texColor0 + (vTexCoord.x, vTexCoord.y, 0.5, .5)));
  //vec4 b = vec4(vTexCoord.x, vTexCoord.y, 0.0, 1);
  vec4 b = vColor;
  
  fragColor = a*b;

}