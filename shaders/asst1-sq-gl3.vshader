#version 150

uniform float uVertexScale;
uniform float uRatioHeight;
uniform float uRatioWidth;

in vec2 aPosition;
in vec2 aTexCoord;

out vec2 vTexCoord;
out vec2 vTemp;

void main() {
	gl_Position = vec4(aPosition.x * uRatioWidth * uVertexScale, aPosition.y * uRatioHeight, 0, 1);
	vTexCoord = aTexCoord;
	vTemp = vec2(1, 1);
}
