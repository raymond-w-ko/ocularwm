uniform sampler2D RT;
varying vec2 uv;

// shamelessly stolen from:
// https://github.com/josh-lieberman/Biclops/blob/master/Scene.fx

// Determines how strong the barrel distortion is, positive values will
// pincushion instead.  The current value is derived from John Carmack D3BFG
// screenshots.
float kBarrelFactor = (-81.0/10.0);

void main()
{   
    vec2 v = vec2(uv);
    v *= 2.0; // [0,1]-based -> [-1,1]
    v -= 1.0;
    vec2 warped = vec2(
        kBarrelFactor * v.x / (v.y * v.y + kBarrelFactor), 
        kBarrelFactor * v.y / (v.x * v.x + kBarrelFactor));
    warped += 1.0; // [-1,1] back into [0,1]
    warped *= 0.5;

    gl_FragColor = texture2D(RT, warped);
}
