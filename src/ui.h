#pragma once

struct VulkanApplication;

struct UI {
private:
    VulkanApplication* application;

public:
    void init(VulkanApplication* application);
    void draw();
};