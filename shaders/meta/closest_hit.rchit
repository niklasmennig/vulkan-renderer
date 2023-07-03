#version 460 core
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable


#define PI 3.1415926535897932384626433832795

float epsilon = 0.0001f;
float ray_max = 1000.0f;

uint max_depth = 6;

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
#line 7

struct RayPayload
{
    vec3 color;
    vec3 contribution;

    uint depth;
    uint seed;
};
#line 8

layout(set = 1, binding = 0) readonly buffer VertexData {vec4 data[];} vertices;
layout(set = 1, binding = 1) readonly buffer VertexIndexData {uint data[];} vertex_indices;
layout(set = 1, binding = 2) readonly buffer NormalData {vec4 data[];} normals;
layout(set = 1, binding = 3) readonly buffer NormalIndexData {uint data[];} normal_indices;
layout(set = 1, binding = 4) readonly buffer TexcoordData {vec2 data[];} texcoords;
layout(set = 1, binding = 5) readonly buffer TexcoordIndexData {uint data[];} texcoord_indices;
layout(set = 1, binding = 6) readonly buffer TangentData {vec4 data[];} tangents;
layout(set = 1, binding = 7) readonly buffer OffsetData {uint data[];} mesh_data_offsets;
layout(set = 1, binding = 8) readonly buffer OffsetIndexData {uint data[];} mesh_offset_indices;

vec3 get_vertex_position(uint instance, vec2 barycentric_coordinates) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 7 + 0];
    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 7 + 1];

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

    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 7 + 2];
    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 7 + 3];

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

    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 7 + 4];
    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 7 + 5];

    vec2 uv0 = texcoords.data[data_offset + texcoord_indices.data[index_offset + idx0]];
    vec2 uv1 = texcoords.data[data_offset + texcoord_indices.data[index_offset + idx1]];
    vec2 uv2 = texcoords.data[data_offset + texcoord_indices.data[index_offset + idx2]];
    vec2 uv = (uv0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y) + uv1 * barycentric_coordinates.x + uv2 * barycentric_coordinates.y);

    return uv;
}

vec4 get_vertex_tangent(uint instance, vec2 barycentric_coordinates) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 7 + 6];
    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 7 + 1];

    vec4 tang0 = tangents.data[data_offset + vertex_indices.data[index_offset + idx0]];
    vec4 tang1 = tangents.data[data_offset + vertex_indices.data[index_offset + idx1]];
    vec4 tang2 = tangents.data[data_offset + vertex_indices.data[index_offset + idx2]];
    vec4 tangent = (tang0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y) + tang1 * barycentric_coordinates.x + tang2 * barycentric_coordinates.y);

    return tang0;
}
#line 9

layout(set = 1, binding = 9) uniform sampler2D tex[16];
layout(set = 1, binding = 10) readonly buffer TextureIndexData {uint data[];} texture_indices;

#define TEXTURE_OFFSET_DIFFUSE 0
#define TEXTURE_OFFSET_NORMAL 1
#define TEXTURE_OFFSET_ROUGHNESS 2

vec3 sample_texture(uint id, vec2 uv) {
    return texture(tex[nonuniformEXT(id)], uv).rgb;
}

vec3 sample_texture(uint instance, vec2 uv, uint offset) {
    return texture(tex[nonuniformEXT(texture_indices.data[instance * 3] + offset)], uv).rgb;
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

struct BSDFSample {
    vec3 direction;
    float pdf;
};

BSDFSample sample_cosine_hemisphere(float u1, float u2)
{
    float theta = 0.5 * acos(-2.0 * u1 + 1.0);
    float phi = u2 * 2.0 * PI;

    float x = cos(phi) * sin(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(theta);

    return BSDFSample(
        vec3(x, y, z),
        1.0 / PI
    );
}

// taken from https://www.cim.mcgill.ca/~derek/ecse689_a3.html
BSDFSample sample_power_hemisphere(float u1, float u2, float n)
{
    float alpha = sqrt(1.0 - pow(u1, 2.0 / (n+1.0)));

    float x = alpha * cos(2.0 * PI * u2);
    float y = alpha * sin(2.0 * PI * u2);
    float z = pow(u1, 1.0 / (n+1.0));

    return BSDFSample(
        vec3(x, y, z),
        1.0 / PI
    );
}
#line 12

//https://www.mathematik.uni-marburg.de/~thormae/lectures/graphics2/graphics_4_3_ger_web.html#1
vec3 fresnel_schlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
} 

float d_ggx(float NoH, float roughness) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float NoH2 = NoH * NoH;
  float b = (NoH2 * (alpha2 - 1.0) + 1.0);
  return alpha2 / (PI * b * b);
}

float g1_ggx_schlick(float NdotV, float roughness) {
  //float r = roughness; // original
  float r = 0.5 + 0.5 * roughness; // Disney remapping
  float k = (r * r) / 2.0;
  float denom = NdotV * (1.0 - k) + k;
  return NdotV / denom;
}

float g_smith(float NoV, float NoL, float roughness) {
  float g1_l = g1_ggx_schlick(NoL, roughness);
  float g1_v = g1_ggx_schlick(NoV, roughness);
  return g1_l * g1_v;
}

vec3 ggx(vec3 ray_in, vec3 ray_out, vec3 normal, vec3 base_color, float metallic, float fresnel_reflect, float roughness) {
    vec3 h = normalize(ray_in + ray_out);

    float no = clamp(dot(normal, ray_out), 0.0, 1.0);
    float ni = clamp(dot(normal, ray_in), 0.0, 1.0);
    float nh = clamp(dot(normal, h), 0.0, 1.0);
    float oh = clamp(dot(ray_out, h), 0.0, 1.0);

    vec3 f0 = vec3(0.16 * (fresnel_reflect * fresnel_reflect));
    f0 = mix(f0, base_color, metallic);

    // specular
    vec3 f = fresnel_schlick(oh, f0);
    float d = d_ggx(nh, roughness);
    float g = g_smith(no, ni, roughness);
    vec3 spec = (d * g * f) / max(4.0 * no * ni, 0.001);

    // diffuse
    vec3 rho = vec3(1.0) - f;
    rho *= 1.0 - metallic;
    vec3 diff = rho * base_color / PI;

    return diff + spec;
}

vec3 sample_ggx(in vec3 V, in vec3 N, 
              in vec3 baseColor, in float metallicness, 
              in float fresnelReflect, in float roughness, in vec3 random, out vec3 nextFactor) 
{
  if(random.z > 0.5) {
    // diffuse case
    
    // important sampling diffuse
    // pdf = cos(theta) * sin(theta) / PI
    float theta = asin(sqrt(random.y));
    float phi = 2.0 * PI * random.x;
    // sampled indirect diffuse direction in normal space
    vec3 localDiffuseDir = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
    vec3 L = basis(N) * localDiffuseDir;  
    
     // half vector
    vec3 H = normalize(V + L);
    float VoH = clamp(dot(V, H), 0.0, 1.0);     
    
    // F0 for dielectics in range [0.0, 0.16] 
    // default FO is (0.16 * 0.5^2) = 0.04
    vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect)); 
    // in case of metals, baseColor contains F0
    f0 = mix(f0, baseColor, metallicness);    
    vec3 F = fresnel_schlick(VoH, f0);
    
    vec3 notSpec = vec3(1.0) - F; // if not specular, use as diffuse
    notSpec *= 1.0 - metallicness; // no diffuse for metals
    vec3 diff = notSpec * baseColor; 
  
    nextFactor = notSpec * baseColor;
    nextFactor *= 2.0; // compensate for splitting diffuse and specular
    return L;
    
  } else {
    // specular case
    
    // important sample GGX
    // pdf = D * cos(theta) * sin(theta)
    float a = roughness * roughness;
    float theta = acos(sqrt((1.0 - random.y) / (1.0 + (a * a - 1.0) * random.y)));
    float phi = 2.0 * PI * random.x;
    
    vec3 localH = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
    vec3 H = basis(N) * localH;  
    vec3 L = reflect(-V, H);

    // all required dot products
    float NoV = clamp(dot(N, V), 0.0, 1.0);
    float NoL = clamp(dot(N, L), 0.0, 1.0);
    float NoH = clamp(dot(N, H), 0.0, 1.0);
    float VoH = clamp(dot(V, H), 0.0, 1.0);     
    
    // F0 for dielectics in range [0.0, 0.16] 
    // default FO is (0.16 * 0.5^2) = 0.04
    vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect)); 
    // in case of metals, baseColor contains F0
    f0 = mix(f0, baseColor, metallicness);
  
    // specular microfacet (cook-torrance) BRDF
    vec3 F = fresnel_schlick(VoH, f0);
    float D = d_ggx(NoH, roughness);
    float G = g_smith(NoV, NoL, roughness);
    nextFactor =  F * G * VoH / max((NoH * NoV), 0.001);
    nextFactor *= 2.0; // compensate for splitting diffuse and specular
    return L;
  } 
}
#line 13

hitAttributeEXT vec2 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(set = 0, binding = 1) uniform accelerationStructureEXT as;

void main() {
    uint instance = gl_InstanceID;

    vec3 position = get_vertex_position(instance, barycentrics);
    vec3 normal = get_vertex_normal(instance, barycentrics);
    vec2 uv = get_vertex_uv(instance, barycentrics);

    vec3 ray_out = normalize(-gl_WorldRayDirectionEXT);
    vec3 new_origin = position;

    //normal mapping
    // vec4 tangent_fsign = get_vertex_tangent(instance, barycentrics);
    // vec3 tangent = tangent_fsign.xyz;
    // // tangent = normalize(vec3(gl_ObjectToWorldEXT * vec4(tangent, 0.0)));
    // vec3 bitangent = normalize(tangent_fsign.w * cross(normal, tangent));
    // mat3 tbn = mat3(tangent, bitangent, normal);

    // vec3 sampled_normal = sample_texture(instance, uv, TEXTURE_OFFSET_NORMAL) * 2.0 - 1.0;
    // //sampled_normal = vec3(1,0,0);
    // payload.color = tangent;
    // return;

    // vec3 mapped_normal = (tbn * sampled_normal);
    // normal = normalize(mapped_normal);

    vec3 base_color = sample_texture(instance, uv, TEXTURE_OFFSET_DIFFUSE);
    vec3 arm = sample_texture(instance, uv, TEXTURE_OFFSET_ROUGHNESS);
    float roughness = arm.g;
    //float roughness = 1.0;
    float metallic = 0.0;
   
    float fresnel_reflect = 0.5;

    // direct light
    vec3 light_position = vec3(1,1,1);
    vec3 light_intensity = vec3(30.0);
    vec3 light_dir = light_position - new_origin;
    float light_dist = length(light_dir);
    light_dir /= light_dist;
    float light_attenuation = 1.0 / (light_dist * light_dist);

    rayQueryEXT ray_query;
    rayQueryInitializeEXT(ray_query, as, gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, new_origin, epsilon, light_dir, light_dist - 2 * epsilon);
    while(rayQueryProceedEXT(ray_query)) {};
    bool in_shadow = (rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionTriangleEXT);

    if (!in_shadow) {
        payload.color += ggx(light_dir, ray_out, normal, base_color, metallic, fresnel_reflect, roughness) * light_intensity * light_attenuation * max(0, dot(light_dir, normal));
    }

    // indirect light
    vec3 next_factor = vec3(0);
    vec3 r = sample_ggx(ray_out, normal, base_color, metallic, fresnel_reflect, roughness, random_vec3(payload.seed), next_factor);

    vec3 new_direction = normalize(r);
    payload.contribution *= next_factor;

    
    if (payload.depth < max_depth) {
        payload.depth += 1;
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