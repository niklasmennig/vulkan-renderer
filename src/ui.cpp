#include "ui.h"

#include "imgui/imgui.h"
#include "vulkan_application.h"

void UI::init(VulkanApplication* app) {
    this->application = app;
}

void UI::draw() {
    changed = false;
    ImGui::Begin("Scene Inspector");

    ImGui::Text("Display");
    static const char* items[]{"Result Image", "Instance Indices", "Albedo"};
    changed |= ImGui::Combo("##display_selector", &displayed_image_index, items, 3);

    ImGui::Text("Application Information");
    ImGui::Text("%.2f FPS", application->get_fps());
    ImGui::Text("%d Samples", application->get_samples());
    ImGui::Text("Mouse Position: %.2f/%.2f", application->get_cursor_position().x, application->get_cursor_position().y);
    ImGui::Text("Color: %d/%d/%d", color_under_cursor.r, color_under_cursor.g, color_under_cursor.b);
    ImGui::Text("Selected Instance: %d", selected_instance);
    if (selected_instance_parameters != nullptr) {
        ImGui::Text("Diffuse");
        changed |= ImGui::SliderFloat3("##diffuse_factor_slider", (float*)&selected_instance_parameters->diffuse_factor, 0.0, 1.0);
        ImGui::Text("Metallic");
        changed |= ImGui::SliderFloat("##metallic_factor_slider", (float*)&selected_instance_parameters->emissive_metallic_factor.a, 0.0, 1.0);
        ImGui::Text("Emission");
        changed |= ImGui::SliderFloat3("##emissive_factor_slider", (float*)&selected_instance_parameters->emissive_metallic_factor.r, 0.0, 10.0);
    }

    ImGui::End();
}

bool UI::has_changed() {
    return changed;
}