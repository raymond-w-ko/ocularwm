uniform sampler2D Texture0;
uniform vec2 LensCenter;
uniform vec2 ScreenCenter;
uniform vec2 Scale;
uniform vec2 ScaleIn;
uniform vec4 HmdWarpParam;
uniform vec4 ChromAbParam;
varying vec2 oUV;
void main()
{
  vec4 oFragColour_1;
  vec4 color_2;
  vec2 tmpvar_3;
  tmpvar_3 = ((oUV - LensCenter) * ScaleIn);
  float tmpvar_4;
  tmpvar_4 = ((tmpvar_3.x * tmpvar_3.x) + (tmpvar_3.y * tmpvar_3.y));
  vec2 tmpvar_5;
  tmpvar_5 = (tmpvar_3 * ((
    (HmdWarpParam.x + (HmdWarpParam.y * tmpvar_4))
   + 
    ((HmdWarpParam.z * tmpvar_4) * tmpvar_4)
  ) + (
    ((HmdWarpParam.w * tmpvar_4) * tmpvar_4)
   * tmpvar_4)));
  vec2 tmpvar_6;
  tmpvar_6 = (LensCenter + (Scale * (tmpvar_5 * 
    (ChromAbParam.z + (ChromAbParam.w * tmpvar_4))
  )));
  if (any(bvec2((
    clamp (tmpvar_6, (ScreenCenter - vec2(0.25, 0.5)), (ScreenCenter + vec2(0.25, 0.5)))
   - tmpvar_6)))) {
    color_2.x = 0.0;
    color_2.y = 0.0;
    color_2.z = 0.0;
    color_2.w = 1.0;
    oFragColour_1 = color_2;
  } else {
    color_2.x = texture2D (Texture0, (LensCenter + (Scale * (tmpvar_5 * 
      (ChromAbParam.x + (ChromAbParam.y * tmpvar_4))
    )))).x;
    color_2.y = texture2D (Texture0, (LensCenter + (Scale * tmpvar_5))).y;
    color_2.z = texture2D (Texture0, tmpvar_6).z;
    color_2.w = 1.0;
    oFragColour_1 = color_2;
  };
  gl_FragData[0] = oFragColour_1;
}

