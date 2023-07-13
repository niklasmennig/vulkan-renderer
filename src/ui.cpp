#include "ui.h"

#include "imgui/imgui.h"
#include "vulkan_application.h"

void UI::init(VulkanApplication* app) {
    this->application = app;
}

void UI::draw() {
    ImGui::Begin("Scene Inspector");

    ImGui::Text("Display");
    static const char* items[]{"Result Image", "Instance Indices", "Albedo"};
    ImGui::Combo("display_selector", &displayed_image_index, items, 3);

    ImGui::Text("Application Information");
    ImGui::Text("%.2f FPS", application->get_fps());
    ImGui::Text("%d Samples", application->get_samples());

    ImGui::End();
}