#include "ui/ui_components.h"
#include <imgui.h>
#include <chrono>
#include <iomanip>
#include <sstream>

void DrawDataWindow(bool& open, UserData& user, std::mutex& mtx) {
    if (!open) return;
    if (ImGui::Begin("Информация о данных", &open)) {
        std::lock_guard<std::mutex> lock(mtx);
        ImGui::Text("Пользователь: %s", user.user.c_str());
        ImGui::Text("Latitude: %.6f", user.location.latitude);
        ImGui::Text("Longitude: %.6f", user.location.longitude);
        ImGui::Text("Altitude: %.2f м", user.location.altitude);
        ImGui::Text("Accuracy: %.2f м", user.location.accuracy);
        
        if (user.location.timestamp > 0) {
            auto tp_ms = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>(
                std::chrono::milliseconds(user.location.timestamp));
            std::time_t time_t_tp = std::chrono::system_clock::to_time_t(tp_ms);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp_ms.time_since_epoch()) % 1000;
            
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t_tp), "%Y-%m-%d %H:%M:%S");
            ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
            ImGui::Text("Timestamp: %s", ss.str().c_str());
        }
        ImGui::Separator();
        for (const auto& n : user.mobileNetworks) {
            ImGui::BulletText("%s | PCI:%d TAC:%d RSRP:%d RSRQ:%d RSSI:%d", n.cellIdentity.c_str(), n.pci, n.tac, n.rsrp, n.rsrq, n.rssi);
        }
    }
    ImGui::End();
}
