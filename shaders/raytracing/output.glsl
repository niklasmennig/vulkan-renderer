#ifndef OUTPUT_GLSL
#define OUTPUT_GLSL

layout(set = DESCRIPTOR_SET_FRAMEWORK, binding = DESCRIPTOR_BINDING_OUTPUT_BUFFERS) buffer OutputBuffer { vec4 color[]; } output_buffers[];

void write_output(uint buffer_id, uint pixel_index, vec4 value) {
    output_buffers[buffer_id].color[pixel_index] = value;
}

vec4 read_output(uint buffer_id, uint pixel_index) {
    return output_buffers[buffer_id].color[pixel_index];
}

#endif