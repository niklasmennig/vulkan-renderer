#include "ui.h"

#include "imgui/imgui.h"
#include "vulkan_application.h"

void UI::init(VulkanApplication* app) {
    this->application = app;
}

void UI::draw() {
    ImGui::Begin("Scene Inspector");

    ImGui::Text("Application Information");
    ImGui::BeginChild("app_info");
    ImGui::Text("%.2f FPS", application->get_fps());
    ImGui::Text("%d Samples", application->get_samples());
    ImGui::EndChild();

    ImGui::End();
}