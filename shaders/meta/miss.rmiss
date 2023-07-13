#version 460 core
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : require

layout(push_constant) uniform PushConstants {
    float time;
    int clear_accumulated;
} push_constants;


#define PI 3.1415926535897932384626433832795
#define NULL_INSTANCE 999999

float epsilon = 0.0001f;
float ray_max = 1000.0f;

uint max_depth = 10;

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
#line 10

// taken from https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
#define PI 3.1415926535897932384626433832795

// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].

float seed_random( inout uint rnd ) { rnd = hash(rnd); return floatConstruct(rnd); }

vec3 random_vec3 (inout uint rnd) { return vec3(seed_random(rnd), seed_random(rnd), seed_random(rnd)); }
#line 11

struct RayPayload
{
    vec3 color;
    vec3 contribution;

    uint depth;
    uint seed;

    uint primary_hit_instance;
    vec3 primary_hit_albedo;
};
#line 12

layout(set = 1, binding = 6) uniform sampler2D tex[16];
layout(set = 1, binding = 7) readonly buffer TextureIndexData {uint data[];} texture_indices;

#define TEXTURE_OFFSET_DIFFUSE 0
#define TEXTURE_OFFSET_NORMAL 1
#define TEXTURE_OFFSET_ROUGHNESS 2

vec3 sample_texture(uint id, vec2 uv) {
    return texture(tex[nonuniformEXT(id)], uv).rgb;
}

vec3 sample_texture(uint instance, vec2 uv, uint offset) {
    return texture(tex[nonuniformEXT(texture_indices.data[instance * 3] + offset)], uv).rgb;
}
#line 13

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {

    vec3 dir = gl_WorldRayDirectionEXT;

    float theta = acos(dir.y);
    float phi = sign(dir.z) * acos(dir.x / sqrt(dir.x * dir.x + dir.z * dir.z));

    float u = phi / (2.0 * PI);
    float v = theta / PI;

    vec3 base_color = sample_texture(0, vec2(u, v));

    payload.color += payload.contribution * base_color;

    if (payload.depth == 0) {
        payload.primary_hit_instance = NULL_INSTANCE;
        payload.primary_hit_albedo = base_color;
    }
}