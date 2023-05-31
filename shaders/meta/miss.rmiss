#version 460 core
#extension GL_EXT_ray_tracing : enable

layout(push_constant) uniform PushConstants {
    float time;
    int clear_accumulated;
} push_constants;


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


// Compound versions of the hashing algorithm I whipped together.
uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
uint hash( uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
uint hash( uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }



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
float random( float x ) { return floatConstruct(hash(floatBitsToUint(x))); }
float random( vec2  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec3  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float random( vec4  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
float seed_random( inout float rnd ) { float val = random(rnd * 3311.432); rnd = val; return val; }
#line 9

struct RayPayload
{
    // ray data
    uint depth;
    float seed;
    bool hit;
    
    // hit data
    uint instance;
    vec3 position;
    vec3 normal;
    vec2 uv;
};


struct MaterialPayload
{
    // ray data
    float seed;

    // input data
    uint instance;
    vec3 position;
    vec3 normal;
    vec2 uv;
    vec3 direction;

    // output data
    vec3 emission;
    vec3 surface_color;
    vec3 sample_direction;
    float sample_pdf;
};
#line 10

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    payload.hit = false;
}