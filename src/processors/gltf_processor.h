#pragma once

#include "../loaders/geometry_gltf.h"

struct GLTFProcessor {
    private:
        GLTFData& data;

    public:
        void set_data(GLTFData& data);
        virtual void process() = 0;
};