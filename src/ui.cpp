#include "ui.h"

#include "imgui/imgui.h"
#include "vulkan_application.h"

void UI::init(VulkanApplication* app) {
    this->application = app;

    selected_output_image = app->get_pipeline().output_image_names[0];
}

void UI::draw() {
    changed = false;
    hovered = false;
    ImGui::Begin("Scene Inspector");
    hovered |= ImGui::IsWindowHovered();

    ImGui::SeparatorText("Camera");
    ImGui::DragFloat("Camera Speed", &camera_speed, camera_speed * 1e-3, 100.0f);
    changed |= ImGui::SliderFloat("Camera FOV", &camera_fov, 1.0, 180.0);
    changed |= ImGui::DragInt("Max Depth", &max_ray_depth, 0.2f, 1, 16);

    ImGui::SeparatorText("Display");

    // output image selection
    if (ImGui::BeginCombo("##display_selector", selected_output_image.c_str())) {
        for (std::string img_name : application->get_pipeline().output_image_names) {
            if (ImGui::Selectable(img_name.c_str())) {
                selected_output_image = img_name;
            }
        }
        ImGui::EndCombo();
    }


    static const char* render_scale_options[]{"Full Resolution", "Half Resolution", "Quarter Resolution"};
    if (ImGui::Combo("##render_scale_selector", &render_scale_index, render_scale_options, sizeof(render_scale_options) / sizeof(char*))) {
        application->set_render_images_dirty();
        changed = true;
    }

    changed |= ImGui::Checkbox("Direct Lighting", &direct_lighting_enabled);
    changed |= ImGui::Checkbox("Indirect Lighting", &indirect_lighting_enabled);

    ImGui::SeparatorText("Application Information");
    ImGui::Text("%.2f FPS", application->get_fps());
    ImGui::Text("%d Samples", application->get_samples());
    ImGui::Text("Mouse Position: %.2f/%.2f", application->get_cursor_position().x, application->get_cursor_position().y);
    ImGui::Text("Color: %d/%d/%d", color_under_cursor.r, color_under_cursor.g, color_under_cursor.b);

    ImGui::SeparatorText("Scene Instance");
    if (selected_instance == -1) {
        ImGui::Text("No instance selected");
    } else {
        ImGui::Text("Selected Instance: %d", selected_instance);
    }
    if (selected_instance_parameters != nullptr) {
        if (ImGui::CollapsingHeader("Instance Editor")) {
            ImGui::BeginChild("instance_editor");
            ImGui::Text("Diffuse");
            changed |= ImGui::ColorPicker4("##diffuse_factor_slider", (float*)&selected_instance_parameters->diffuse_opacity);
            ImGui::Text("Roughness");
            changed |= ImGui::SliderFloat("##roughness_factor_slider", (float*)&selected_instance_parameters->roughness_metallic_transmissive_ior.x, 0.0, 1.0);
            ImGui::Text("Metallic");
            changed |= ImGui::SliderFloat("##metallic_factor_slider", (float*)&selected_instance_parameters->roughness_metallic_transmissive_ior.y, 0.0, 1.0);
            ImGui::Text("Emission Color");
            changed |= ImGui::ColorPicker3("##emissive_color_picker", (float*)&selected_instance_parameters->emissive_factor);
            ImGui::Text("Emission Strength");
            changed |= ImGui::SliderFloat("##emissive_factor_slider", (float*)&selected_instance_parameters->emissive_factor.a, 0.0, 100.0);
            ImGui::Text("Transmission");
            changed |= ImGui::SliderFloat("##transmissive_factor_slider", (float*)&selected_instance_parameters->roughness_metallic_transmissive_ior.z, 0.0, 1.0);
            ImGui::Text("IOR");
            changed |= ImGui::SliderFloat("##transmissive_ior_slider", (float*)&selected_instance_parameters->roughness_metallic_transmissive_ior.a, 1.0, 3.0);
            ImGui::EndChild();
        }
    }

    ImGui::End();
    hovered |= ImGui::IsAnyItemHovered() | ImGui::IsAnyItemFocused() | ImGui::IsAnyItemActive();
}

bool UI::has_changed() {
    return changed;
}

bool UI::is_hovered() {
    return hovered;
}