#ifndef LIGHTS_GLSL
#define LIGHTS_GLSL

#include "common.glsl"
#include "interface.glsl"
#include "mesh_data.glsl"
#include "texture_data.glsl"
#include "material.glsl"

struct LightSample {
    vec3 direction;
    float distance;
    vec3 intensity; // already divided by PDF
    float pdf;
};

float pdf_area_light(uint instance, uint primitive, mat4x3 transform) {
    vec3 v0, v1, v2;
    get_vertices(instance, primitive, v0, v1, v2);
    v0 = transform * vec4(v0, 1.0);
    v1 = transform * vec4(v1, 1.0);
    v2 = transform * vec4(v2, 1.0);
    float area = dot(v1-v0, v2-v0);
    return 1.0 / area;
}

LightSample sample_light(vec3 position, inout uint seed, Light light) {
    LightSample light_sample;
    uint type = light.uint_data[0];
    vec3 direction;
    float distance;
    switch (type) {
        case 0: // POINT LIGHT
            direction = vec3(light.float_data[0], light.float_data[1], light.float_data[2]) - position;
            distance = length(direction);
            light_sample.direction = direction / distance;
            light_sample.distance = distance;
            float attenuation = pow(1.0 / distance, 2.0);
            light_sample.intensity = max(vec3(0), vec3(light.float_data[3], light.float_data[4], light.float_data[5]) * attenuation);
            light_sample.pdf = 1.0;
            break;
        case 1: // AREA LIGHT
            uint instance = light.uint_data[1];
            uint vertex_count = light.uint_data[2];
            uint primitive_count = vertex_count / 3;

            uint primitive = uint(floor(random_float(seed) * primitive_count));
            vec3 light_position = get_vertex_position(instance, primitive, vec2(random_float(seed), random_float(seed)));

            mat4x3 transform;
            transform[0][0] = light.float_data[0];
            transform[1][0] = light.float_data[1];
            transform[2][0] = light.float_data[2];
            transform[3][0] = light.float_data[3];
            transform[0][1] = light.float_data[4];
            transform[1][1] = light.float_data[5];
            transform[2][1] = light.float_data[6];
            transform[3][1] = light.float_data[7];
            transform[0][2] = light.float_data[8];
            transform[1][2] = light.float_data[9];
            transform[2][2] = light.float_data[10];
            transform[3][2] = light.float_data[11];
            // transform[0][3] = light.float_data[12];
            // transform[1][3] = light.float_data[13];
            // transform[2][3] = light.float_data[14];
            // transform[3][3] = light.float_data[15];

            light_position = transform * vec4(light_position, 1.0);

            direction = light_position - position;
            distance = length(direction);

            light_sample.direction = direction / distance;
            light_sample.distance = distance;

            float pdf = 1.0 / (pdf_area_light(instance, primitive, transform) / primitive_count);

            MaterialParameters material_parameters = get_material_parameters(instance);
            vec2 uv = get_vertex_uv(instance, primitive, vec2(random_float(seed), random_float(seed)));
            light_sample.intensity = (sample_texture(instance, uv, TEXTURE_OFFSET_EMISSIVE).rgb * material_parameters.emissive) * material_parameters.emission_strength / pdf;
            light_sample.pdf = pdf;
            break;
        case 2: // DIRECTIONAL LIGHT
            light_sample.direction = -normalize(vec3(light.float_data[0], light.float_data[1], light.float_data[2]));
            light_sample.distance = FLT_MAX;
            light_sample.intensity = max(vec3(0), vec3(light.float_data[3], light.float_data[4], light.float_data[5]));
            light_sample.pdf = FLT_MAX;
            break;
    }
    return light_sample;
}
#endif