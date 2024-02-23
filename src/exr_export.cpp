#include <iostream>

#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 1
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

void save_exr_image(const char* path, void* data, uint32_t width, uint32_t height, uint32_t stride) {
    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage exr_image;
    InitEXRImage(&exr_image);

    std::cout << "saving" << std::endl;

    exr_image.num_channels = 3;

    std::vector<float> channels[3];
    channels[0].resize(width * height);
    channels[1].resize(width * height);
    channels[2].resize(width * height);

    for (int i = 0; i < width * height; i++) {
        float* pixel_data = reinterpret_cast<float*>((uint8_t*)data + i * stride);

        channels[0][i] = pixel_data[0];
        channels[1][i] = pixel_data[1];
        channels[2][i] = pixel_data[2];
    }

    float* channel_ptrs[3];
    // exr stored in BGR format
    channel_ptrs[0] = channels[2].data();
    channel_ptrs[1] = channels[1].data();
    channel_ptrs[2] = channels[0].data();

    exr_image.images = (unsigned char**) channel_ptrs;
    exr_image.width = width;
    exr_image.height = height;

    header.num_channels = 3;
    header.channels = (EXRChannelInfo*) malloc(sizeof(EXRChannelInfo) * header.num_channels);

    strncpy(header.channels[0].name, "B", 255); header.channels[0].name[strlen("B")] = '\0';
    strncpy(header.channels[1].name, "G", 255); header.channels[1].name[strlen("G")] = '\0';
    strncpy(header.channels[2].name, "R", 255); header.channels[2].name[strlen("R")] = '\0';

    header.pixel_types = (int*) malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types = (int*) malloc(sizeof(int) * header.num_channels);

    for (int i = 0; i < header.num_channels; i++) {
        header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
    }

    const char* err = NULL;

    std::cout << "saving EXR image" << std::endl;

    int ret = SaveEXRImageToFile(&exr_image, &header, path, &err);
    if (ret != TINYEXR_SUCCESS) {
        std::cout << "Error saving exr image: " << err << std::endl;
        FreeEXRErrorMessage(err);
    }

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);
}