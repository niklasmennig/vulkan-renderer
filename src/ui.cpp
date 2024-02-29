#include "ui.h"

#include <format>

#include "imgui/imgui.h"
#include "vulkan_application.h"

void UI::init(VulkanApplication* app) {
    this->application = app;

    selected_output_image = app->get_pipeline().created_output_buffers[0].name;
}

void UI::draw() {
    changed = false;
    hovered = false;
    render_scale_changed = false;

    ImGui::Begin("Scene Inspector");
    hovered |= ImGui::IsWindowHovered();
    ImGui::DragFloat("Exposure", &exposure, 0.1);

    ImGui::SeparatorText("Camera");
    ImGui::DragFloat("Camera Speed", &camera_speed, camera_speed * 1e-3, 100.0f);
    changed |= ImGui::SliderFloat("Camera FOV", &camera_fov, 1.0, 180.0);
    changed |= ImGui::DragInt("Max Depth", &max_ray_depth, 0.2f, 1, 16);
    changed |= ImGui::DragInt("Frame Samples", &frame_samples, 0.2f, 1, 64);
    
    ImGui::SeparatorText("Display");

    // output image selection
    if (ImGui::BeginCombo("##display_selector", selected_output_image.c_str())) {
        for (OutputBuffer out : application->get_pipeline().created_output_buffers) {
            if (!out.hidden) {
                if (ImGui::Selectable(out.name.c_str())) {
                    selected_output_image = out.name;
                }
            }
        }
        ImGui::EndCombo();
    }


    render_scale_changed |= ImGui::SliderFloat("Render Scale", &render_scale, 0.1, 1.0);

    changed |= ImGui::Checkbox("Direct Lighting", &direct_lighting_enabled);
    changed |= ImGui::Checkbox("Indirect Lighting", &indirect_lighting_enabled);
    changed |= ImGui::Checkbox("ReSTIR Direct Lighting", &restir_enabled);

    changed |= ImGui::Checkbox("Use Processing Pipeline", &use_processing_pipeline);
    
    if (ImGui::Button("Save Screenshot")) {
        application->save_screenshot("screenshot.exr");
    }

    if (ImGui::Button("Rebuild Pipeline")) {
        application->set_pipeline_dirty();
        changed = true;
    }

    ImGui::SeparatorText("Application Information");
    ImGui::Text("%.2f FPS", application->get_fps());
    ImGui::Text("%d Samples", application->get_samples());
    ImGui::Text("Mouse Position: %.2f/%.2f", application->get_cursor_position().x, application->get_cursor_position().y);
    ImGui::Text("Color: %.2f/%.2f/%.2f", color_under_cursor.r, color_under_cursor.g, color_under_cursor.b);

    // realtime editing of light sources (point lights)
    ImGui::SeparatorText("Light Sources");
    auto scene_lights = application->get_scene_data().lights;
    auto& light_data = application->get_lights();
    for (int i = 0; i < scene_lights.size(); i++) {
        auto light = scene_lights[i];
        // area lights are edited via the instance editor
        if (light.type == LightData::LightType::AREA) continue;
        auto& data = light_data[i];
        if (ImGui::CollapsingHeader(light.name.c_str())) {
            if (light.type == LightData::LightType::POINT) {
                changed |= ImGui::DragFloat3("Position", (float*)&data.float_data);
                changed |= ImGui::DragFloat3("Intensity", (float*)&data.float_data[3]);
            } else if (light.type == LightData::LightType::DIRECTIONAL) {
                changed |= ImGui::DragFloat3("Direction", (float*)&data.float_data);
                changed |= ImGui::DragFloat3("Intensity", (float*)&data.float_data[3]);
            }
        }
    }

    // realtime editing of instance material parameters
    ImGui::SeparatorText("Scene Instance");
    if (selected_instance == -1) {
        ImGui::Text("No instance selected");
    } else {
        ImGui::Text("Selected Instance: %d", selected_instance);
    }
    if (selected_instance_parameters != nullptr) {
        if (ImGui::CollapsingHeader("Instance Editor")) {
            //ImGui::BeginChild("instance_editor");
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
            changed |= ImGui::SliderFloat("##transmissive_ior_slider", (float*)&selected_instance_parameters->roughness_metallic_transmissive_ior.a, 1.0, 15.0);
            //ImGui::EndChild();
        }
    }

    ImGui::End();
    hovered |= ImGui::IsAnyItemHovered() | ImGui::IsAnyItemFocused() | ImGui::IsAnyItemActive();
}

bool UI::has_changed() {
    return changed;
}

bool UI::has_render_scale_changed() {
    return render_scale_changed;
}

bool UI::is_hovered() {
    return hovered;
}