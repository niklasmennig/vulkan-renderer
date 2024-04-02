#ifndef COMMON_GLSL
#define COMMON_GLSL

#define FLT_MAX 30000000000000000000000.0f

#define PI 3.1415926

#define EPSILON 0.00001
#define RAY_LEN_MAX 100000.0

#include "push_constants.glsl"

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
    n = normalize(n);

    return mat3(f, n, r);
}

// https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color
float luminance(vec3 color) {
    return (0.299*color.r + 0.587*color.g + 0.114*color.b);
}

vec3 dir_from_thetaphi(float theta, float phi) {
    float x = sin(theta) * cos(phi);
    float y = cos(theta);
    float z = sin(theta) * sin(phi);

    return normalize(vec3(x,y,z));
}

vec2 thetaphi_from_dir(vec3 direction) {
    float theta = acos(direction.y / length(direction));
    float phi = sign(direction.z) * acos(direction.x / length(direction.xz));

    return vec2(theta, phi);
}

vec3 encode_uint(uint to_encode) {
    return vec3(float(uint(float(to_encode) / (255.0 * 255.0)) % 255) / 255.0, float(uint(float(to_encode) / 255.0) % 255) / 255.0, float(to_encode % 255) / 255.0);
}

uint decode_uint(vec3 encoded) {
    return uint(encoded.z * 255) + uint(encoded.y * (255 * 255)) + uint(encoded.x * (255 * 255 * 255));
}

uint pixel_to_index(uvec2 pixel_coordinates) {
    return pixel_coordinates.x + pixel_coordinates.y * get_push_constants().render_extent.x;
}

vec2 pixel_to_ndc(vec2 pixel_coordinates) {
    return ((pixel_coordinates / get_push_constants().render_extent) * 2.0) - 1.0;
}

vec2 ndc_to_pixel(vec2 ndc) {
    return ((ndc + 1.0) / 2.0) * get_push_constants().render_extent;
}

#endif