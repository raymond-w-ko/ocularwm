attribute vec4 vertex;
attribute vec4 uv0;

varying vec2 oUV;

uniform mat4 WorldViewProjection;

void main() {
	gl_Position = WorldViewProjection * vertex;
	oUV = uv0.xy;
}
