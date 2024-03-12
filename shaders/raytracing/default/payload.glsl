#ifndef PAYLOAD_GLSL
#define PAYLOAD_GLSL

struct RayPayload {
    vec3 color;
    uint depth;
    uint pixel_index;

    uint restir_index;

    vec3 contribution;

    vec3 origin;
    vec3 direction;

    uint seed;

    uint primary_hit_instance;
    vec3 primary_hit_position;
    vec2 primary_hit_uv;
    vec3 primary_hit_albedo;
    vec3 primary_hit_normal;
    float primary_hit_roughness;

    float environment_cdf;
    float environment_conditional;

    float last_bsdf_pdf_inv;
};

#endif