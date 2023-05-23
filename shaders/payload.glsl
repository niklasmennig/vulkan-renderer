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

    // hit data
    uint instance;
    vec3 position;
    vec3 normal;
    vec2 uv;

    // output data
    vec3 emission;
    vec3 surface_color;
    vec3 direction;
};