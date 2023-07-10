#pragma once

struct VulkanApplication;

struct UI {
private:
    VulkanApplication* application;

public:
    int displayed_image_index = 0;

    void init(VulkanApplication* application);
    void draw();
};