#ifndef INC_INTERFACE_P
#define INC_INTERFACE_P

#define DESCRIPTOR_BINDING_BUFFERS 0
#define DESCRIPTOR_BINDING_IMAGES 1

struct PushConstants
{
    ivec2 swapchain_extent;
    ivec2 render_extent;
};


#endif