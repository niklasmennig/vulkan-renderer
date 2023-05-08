struct RayPayload
{
    float hit_t;
    vec3 hit_position;
    vec3 hit_normal;
    vec2 hit_uv;
    uint hit_instance;
    uint hit_primitive;
};