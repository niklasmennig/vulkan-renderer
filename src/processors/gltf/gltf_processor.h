#pragma once

#include "../loaders/geometry_gltf.h"

// structure intended to perform changes on loaded GLTF data
struct GLTFProcessor {
    private:
        GLTFData* data;

    public:
        void set_data(GLTFData* data);
        virtual void process() = 0;
};