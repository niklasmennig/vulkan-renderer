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