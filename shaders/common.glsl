#define PI 3.1415926535897932384626433832795

float epsilon = 0.001f;
float ray_max = 1000.0f;


// taken from https://www.shadertoy.com/view/tlVczh
void basis(in vec3 n, out vec3 f, out vec3 r)
{
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
}