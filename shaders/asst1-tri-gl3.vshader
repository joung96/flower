#version 150

uniform float uVertexScale;
uniform float uRatioHeight;
uniform float uRatioWidth;
uniform float uXmove;
uniform float uYmove;

in vec2 aPosition;
in vec2 aTexCoord;
in vec4 aColor;

out vec2 vTexCoord;
out vec2 vTemp;
out vec4 vColor;

void main() {
  gl_Position = vec4(((aPosition.x + uYmove) * uRatioWidth * uVertexScale), ((aPosition.y + uXmove) * uRatioHeight), 0, 1);
  vTexCoord = aTexCoord;
  vTemp = vec2(1,1);
  vColor = aColor;
}
