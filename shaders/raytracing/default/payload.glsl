#ifndef PAYLOAD_GLSL
#define PAYLOAD_GLSL

struct RayPayload {
    vec3 color;
    uint depth;

    vec3 contribution;
    uint pixel_index;

    vec3 origin;
    uint seed;
    
    vec3 direction;
    float last_bsdf_pdf_inv;


    vec3 primary_hit_position;

    vec2 primary_hit_uv;

};

#endif