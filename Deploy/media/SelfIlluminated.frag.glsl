uniform sampler2D Texture0;
varying vec2 oUV;
void main() {
  gl_FragColor = texture2D(Texture0, oUV);
}
