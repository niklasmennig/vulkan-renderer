#version 460 core
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable


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
#line 5

struct RayPayload
{
    vec3 color;
    vec3 contribution;

    uint depth;
    uint seed;
};
#line 6

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
    vec3 vert = (vert0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y) + vert1 * barycentric_coordinates.x + vert2 * barycentric_coordinates.y);

    return vec3(gl_ObjectToWorldEXT * vec4(vert, 1.0));
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
    vec3 norm = (norm0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y) + norm1 * barycentric_coordinates.x + norm2 * barycentric_coordinates.y);

    return normalize(vec3(gl_ObjectToWorldEXT * vec4(norm, 0.0)));
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
    vec2 uv = (uv0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y) + uv1 * barycentric_coordinates.x + uv2 * barycentric_coordinates.y);

    return uv;
}
#line 7

layout(set = 1, binding = 8) uniform sampler2D tex[16];
layout(set = 1, binding = 9) readonly buffer TextureIndexData {uint data[];} texture_indices;

#define TEXTURE_OFFSET_DIFFUSE 0
#define TEXTURE_OFFSET_NORMAL 1
#define TEXTURE_OFFSET_ROUGHNESS 2

vec3 sample_texture(uint id, vec2 uv) {
    return texture(tex[id], uv).rgb;
}

vec3 sample_texture(uint instance, vec2 uv, uint offset) {
    return texture(tex[texture_indices.data[instance * 3] + offset], uv).rgb;
}
#line 8

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
float random( float x ) { return floatConstruct(hash(floatBitsToUint(x))); }

float seed_random( inout uint rnd ) { rnd = hash(rnd); return floatConstruct(rnd); }

vec3 random_point_in_unit_sphere(inout uint rnd) {
    while (true) {
        vec3 p = vec3(seed_random(rnd) * 2.0 - 1.0, seed_random(rnd) * 2.0 - 1.0, seed_random(rnd) * 2.0 - 1.0);
        if (length(p) <= 1) {
            return p;
        }
    }
}
#line 9

vec3 sample_cosine_hemisphere(float u1, float u2)
{
    float theta = 0.5 * acos(-2.0 * u1 + 1.0);
    float phi = u2 * 2.0 * PI;

    float x = cos(phi) * sin(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(theta);

    return vec3(x, y, z);
}

// taken from https://www.cim.mcgill.ca/~derek/ecse689_a3.html
vec3 sample_power_hemisphere(float u1, float u2, float n)
{
    float alpha = sqrt(1.0 - pow(u1, 2.0 / (n+1.0)));

    float x = alpha * cos(2.0 * PI * u2);
    float y = alpha * sin(2.0 * PI * u2);
    float z = pow(u1, 1.0 / (n+1.0));

    return vec3(x, y, z);
}
#line 10

hitAttributeEXT vec2 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(set = 0, binding = 1) uniform accelerationStructureEXT as;

void main() {
    uint instance = gl_InstanceID;
    vec3 position = get_vertex_position(instance, barycentrics);
    vec3 normal = get_vertex_normal(instance, barycentrics);
    vec2 uv = get_vertex_uv(instance, barycentrics);

    vec3 ray_out = -gl_WorldRayDirectionEXT;
    vec3 new_origin = position;

    vec3 t, bt;
    basis(normal, t, bt);

    //normal mapping
    mat3 tbn = mat3(t,bt,normal);
    vec3 sampled_normal = sample_texture(instance, uv, TEXTURE_OFFSET_NORMAL) * 2.0 - 1.0;
    vec3 mapped_normal = normalize(tbn * sampled_normal);
    normal = mapped_normal;

    vec3 base_color = sample_texture(instance, uv, TEXTURE_OFFSET_DIFFUSE);
    float roughness = sample_texture(instance, uv, TEXTURE_OFFSET_ROUGHNESS).g;

    // direct light
    vec3 light_position = vec3(5,0,1);
    vec3 light_intensity = vec3(50.0);
    vec3 light_dir = light_position - new_origin;
    float light_dist = length(light_dir);
    light_dir /= light_dist;
    float light_attenuation = 1.0 / (light_dist * light_dist);

    rayQueryEXT ray_query;
    rayQueryInitializeEXT(ray_query, as, gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, new_origin + normal * epsilon, 0, light_dir, light_dist - epsilon);
    while(rayQueryProceedEXT(ray_query)) {};
    bool in_shadow = (rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionTriangleEXT);

    if (!in_shadow) {
        payload.color += base_color * light_intensity * light_attenuation * max(0, dot(light_dir, normal));
    }

    // indirect light
    vec3 sample_reflection = sample_cosine_hemisphere(seed_random(payload.seed), seed_random(payload.seed));
    float pdf = 1.0 / PI;
    
    vec3 new_direction = normalize(t * sample_reflection.x + bt * sample_reflection.y + normal * sample_reflection.z);

    payload.contribution *= base_color * max(0, dot(new_direction, normal)) / pdf;

    if (payload.depth < max_depth) {
        payload.depth = payload.depth + 1;
        traceRayEXT(
                as,
                gl_RayFlagsOpaqueEXT,
                0xff,
                0,
                0,
                0,
                new_origin,
                epsilon,
                new_direction,
                ray_max,
                0
            );
    }

}