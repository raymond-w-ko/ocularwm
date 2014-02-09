attribute vec4 vertex;
attribute vec4 uv0;

varying vec2 oUV;

void main() {
	gl_Position = vertex;
	oUV = uv0.xy;
}
