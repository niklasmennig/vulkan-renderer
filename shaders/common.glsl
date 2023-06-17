#define PI 3.1415926535897932384626433832795

float epsilon = 0.0001f;
float ray_max = 1000.0f;

uint max_depth = 6;

// taken from https://www.shadertoy.com/view/tlVczh
void basis(in vec3 n, out vec3 f, out vec3 r)
{
   //looks good but has this ugly branch
  if(n.y < -0.99995)
    {
        f = vec3(0.0 , 0.0, -1.0);
        r = vec3(-1.0, 0.0, 0.0);
    }
    else
    {
    	float a = 1.0/(1.0 + n.y);
    	float b = -n.x*n.z*a;
    	f = vec3(1.0 - n.x*n.x*a, b, -n.x);
    	r = vec3(b, 1.0 - n.y*n.z*a , -n.z);
    }

    f = normalize(f);
    r = normalize(r);
}

// https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color
float luminance(vec3 color) {
    return (0.299*color.r + 0.587*color.g + 0.114*color.b);
}