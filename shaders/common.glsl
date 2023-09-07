#ifndef COMMON_GLSL
#define COMMON_GLSL

#define PI 3.1415926
#define NULL_INSTANCE 999999

float epsilon = 0.0001f;
float ray_max = 100000.0f;

// taken from https://www.shadertoy.com/view/tlVczh
mat3 basis(in vec3 n)
{
    vec3 f, r;
   //looks good but has this ugly branch
  if(n.z < -0.99995)
    {
        f = vec3(0 , -1, 0);
        r = vec3(-1, 0, 0);
    }
    else
    {
    	float a = 1./(1. + n.z);
    	float b = -n.x*n.y*a;
    	f = vec3(1. - n.x*n.x*a, b, -n.x);
    	r = vec3(b, 1. - n.y*n.y*a , -n.y);
    }

    f = normalize(f);
    r = normalize(r);

    return mat3(f, r, n);
}

// https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color
float luminance(vec3 color) {
    return (0.299*color.r + 0.587*color.g + 0.114*color.b);
}

#endif