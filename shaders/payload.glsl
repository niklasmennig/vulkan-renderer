struct RayPayload
{
    vec3 color;
    vec3 contribution;

    uint depth;
};


struct MaterialPayload
{
    // ray data
    uint seed;

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