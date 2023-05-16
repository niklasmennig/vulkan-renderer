#version 460 core
#extension GL_EXT_ray_tracing : enable

precision highp float;


#define PI 3.1415926535897932384626433832795

float epsilon = 0.001f;
float ray_max = 1000.0f;

uint max_bounces = 6;


// taken from https://www.shadertoy.com/view/tlVczh
void basis(in vec3 n, out vec3 f, out vec3 r)
{
   //looks good but has this ugly branch
  if(n.z < -0.99995)
    {
        f = vec3(0.0 , -1.0, 0.0);
        r = vec3(-1.0, 0.0, 0.0);
    }
    else
    {
    	float a = 1.0/(1.0 + n.z);
    	float b = -n.x*n.y*a;
    	f = vec3(1.0 - n.x*n.x*a, b, -n.x);
    	r = vec3(b, 1.0 - n.y*n.y*a , -n.y);
    }

    f = normalize(f);
    r = normalize(r);
}
layout(set = 1, binding = 0) readonly buffer VertexData {vec4 data[];} vertices;
layout(set = 1, binding = 1) readonly buffer VertexIndexData {uint data[];} vertex_indices;
layout(set = 1, binding = 2) readonly buffer NormalData {vec4 data[];} normals;
layout(set = 1, binding = 3) readonly buffer NormalIndexData {uint data[];} normal_indices;
layout(set = 1, binding = 4) readonly buffer TexcoordData {vec2 data[];} texcoords;
layout(set = 1, binding = 5) readonly buffer TexcoordIndexData {uint data[];} texcoord_indices;
layout(set = 1, binding = 6) readonly buffer OffsetData {uint data[];} mesh_data_offsets;
layout(set = 1, binding = 7) readonly buffer OffsetIndexData {uint data[];} mesh_offset_indices;

vec3 get_vertex_position(uint instance, vec2 barycentric_coordinates) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 6 + 0];
    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 6 + 1];

    vec3 vert0 = vertices.data[data_offset + vertex_indices.data[index_offset + idx0]].xyz;
    vec3 vert1 = vertices.data[data_offset + vertex_indices.data[index_offset + idx1]].xyz;
    vec3 vert2 = vertices.data[data_offset + vertex_indices.data[index_offset + idx2]].xyz;
    vec3 vert = (vert1 * barycentric_coordinates.x + vert2 * barycentric_coordinates.y + vert0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y));

    return vert;
}

vec3 get_vertex_normal(uint instance, vec2 barycentric_coordinates) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 6 + 2];
    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 6 + 3];

    vec3 norm0 = normals.data[data_offset + normal_indices.data[index_offset + idx0]].xyz;
    vec3 norm1 = normals.data[data_offset + normal_indices.data[index_offset + idx1]].xyz;
    vec3 norm2 = normals.data[data_offset + normal_indices.data[index_offset + idx2]].xyz;
    vec3 norm = (norm1 * barycentric_coordinates.x + norm2 * barycentric_coordinates.y + norm0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y));

    return normalize(norm);
}

vec2 get_vertex_uv(uint instance, vec2 barycentric_coordinates) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 6 + 4];
    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 6 + 5];

    vec2 uv0 = texcoords.data[data_offset + texcoord_indices.data[index_offset + idx0]];
    vec2 uv1 = texcoords.data[data_offset + texcoord_indices.data[index_offset + idx1]];
    vec2 uv2 = texcoords.data[data_offset + texcoord_indices.data[index_offset + idx2]];
    vec2 uv = (uv1 * barycentric_coordinates.x + uv2 * barycentric_coordinates.y + uv0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y));

    return uv;
}
layout(set = 1, binding = 8) uniform sampler2D tex[16];
layout(set = 1, binding = 9) readonly buffer TextureIndexData {uint data[];} texture_indices;

vec3 sample_texture(uint instance, vec2 uv) {
    return texture(tex[texture_indices.data[instance]], uv).rgb;
}
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
struct RayPayload
{
    uint depth;
    vec3 contribution;
    float seed;
};

struct MaterialPayload
{
    // provided when calling
    uint instance;
    vec2 uv;
    vec3 position;
    vec3 normal;
    float seed;
    
    // returned from material shader
    vec3 emission;
    vec3 surface_color;
    vec3 reflection_direction;
};
hitAttributeEXT vec2 barycentrics;

layout(set = 0, binding = 1) uniform accelerationStructureEXT as;


layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 0) callableDataEXT MaterialPayload material_payload;

void main() {

    if (payload.depth > max_bounces) {
        payload.contribution = vec3(1.0);
        return;
    }

    // indices into mesh data
    vec3 normal = get_vertex_normal(gl_InstanceID, barycentrics);
    vec2 uv = get_vertex_uv(gl_InstanceID, barycentrics);
    vec3 hit_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

    payload.depth += 1;

    vec3 emission = vec3(0.0);
    if (gl_InstanceID == 0) emission = vec3(10.0);

    float cosine_term = max(0, dot(gl_WorldRayDirectionEXT, -normal));

    material_payload.instance = gl_InstanceID;
    material_payload.position = hit_position;
    material_payload.normal = normal;
    material_payload.uv = uv;
    material_payload.seed = payload.seed;

    //executeCallableEXT(0, 0);

    vec3 ray_origin = hit_position;
    vec3 ray_direction = material_payload.reflection_direction;

    traceRayEXT(
                as,
                gl_RayFlagsOpaqueEXT,
                0xff,
                0,
                0,
                0,
                ray_origin,
                epsilon,
                ray_direction,
                ray_max,
                0
            );

    vec3 irradiance = payload.contribution * max(0, dot(ray_direction, normal));

    vec3 radiance = material_payload.emission + material_payload.surface_color * irradiance * cosine_term;

    payload.contribution = radiance;
}