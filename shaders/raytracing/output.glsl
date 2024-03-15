#ifndef OUTPUT_GLSL
#define OUTPUT_GLSL

layout(set = DESCRIPTOR_SET_FRAMEWORK, binding = DESCRIPTOR_BINDING_OUTPUT_BUFFERS) buffer OutputBuffer { vec4 color[]; } output_buffers[];

void write_output(uint buffer_id, uint pixel_index, vec4 value) {
    output_buffers[buffer_id].color[pixel_index] = value;
}

vec4 read_output(uint buffer_id, uint pixel_index) {
    return output_buffers[buffer_id].color[pixel_index];
}

vec3 encode_uint(uint to_encode) {
    return vec3(float(uint(float(to_encode) / (255.0 * 255.0)) % 255) / 255.0, float(uint(float(to_encode) / 255.0) % 255) / 255.0, float(to_encode % 255) / 255.0);
}

uint decode_uint(vec3 encoded) {
    return uint(encoded.z * 255) + uint(encoded.y * (255 * 255)) + uint(encoded.x * (255 * 255 * 255));
}

#endif