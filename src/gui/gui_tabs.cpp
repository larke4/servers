#include "gui_tabs.h"
#include "ui/ui_components.h"
#include "osm_math.h"
#include "net/zmq_receiver.h"
#include <imgui.h>
#include <implot.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <cstring>
#include <limits>
#include <cmath>
#include "logging.h"

using json = nlohmann::json;
using namespace osm14;

static std::string criterion_to_string(HeatmapCriterion c) {
    switch(c) {
        case HeatmapCriterion::RSRP: return "RSRP";
        case HeatmapCriterion::RSRQ: return "RSRQ";
        case HeatmapCriterion::RSSI: return "RSSI";
        case HeatmapCriterion::SINR: return "SINR";
        case HeatmapCriterion::Altitude: return "Altitude";
        default: return "Unknown";
    }
}

void TabMain(RuntimeState& st) {
    bool open = true;
    DrawDataWindow(open, st.currentUser, st.mtx);
    std::lock_guard<std::mutex> lk(st.mtx);
    ImGui::SeparatorText("Позиция");
    ImGui::Text("LAT: %.6f | LON: %.6f", st.currentUser.location.latitude, st.currentUser.location.longitude);
}

void TabSignals(RuntimeState& st) {
    bool open = true; static std::string sel;
    DrawGraphWindow(open, st.signalSeries, sel, st.mtx);
}

void TabLocal(RuntimeState& st) {
    if (ImGui::Button("Загрузить drive_test_log.json")) {
        std::lock_guard<std::mutex> lk(st.localData.mtx);
        st.localData.pciTime.clear(); st.localData.pciRsrp.clear(); st.localData.trajLat.clear(); st.localData.trajLon.clear();
        std::ifstream f("drive_test_log.json");
        std::string line; long long t0 = -1;
        while (std::getline(f, line)) {
            try {
                auto j = json::parse(line);
                long long ts = j.value("timestamp", 0LL);
                if (ts == 0) ts = j["location"].value("time", 0LL);
                if (t0 < 0) t0 = ts;
                double t = (double)(ts - t0) / 1000.0;
                st.localData.trajLat.push_back(j["location"]["latitude"]);
                st.localData.trajLon.push_back(j["location"]["longitude"]);
                for (auto& c : j["telephony"]) {
                    int pci = c.value("pci", -1);
                    if (pci == -1 && c.contains("CellIdentityLte")) pci = c["CellIdentityLte"].value("PCI", -1);
                    if (pci == -1) continue;
                    double rsrp = c.value("rsrp", -140.0);
                    if (rsrp == -140 && c.contains("CellSignalStrengthLte")) rsrp = (double)c["CellSignalStrengthLte"].value("RSRP", -140);
                    st.localData.pciTime[pci].push_back(t);
                    st.localData.pciRsrp[pci].push_back(rsrp);
                }
            } catch (...) {}
        }
        LogInfo("drive_test_log.json loaded: traj points=" + std::to_string(st.localData.trajLat.size()) + " pci series=" + std::to_string(st.localData.pciTime.size()));
        st.localData.loaded = true;
    }
    if (!st.localData.loaded) return;
    if (ImPlot::BeginPlot("##LocalRSRP", ImVec2(-1, 400))) {
        ImPlot::SetupAxes("Время (с)", "RSRP (дБм)");
        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Horizontal);
        for (auto& [pci, times] : st.localData.pciTime) {
            auto& rsrp = st.localData.pciRsrp[pci];
            if (times.empty()) continue;

            std::vector<double> t_plot, v_plot;
            t_plot.reserve(times.size() * 1.1);
            v_plot.reserve(times.size() * 1.1);

            for (size_t i = 0; i < times.size(); ++i) {
                if (i > 0 && (times[i] - times[i-1]) > 600.0) {
                    t_plot.push_back(std::numeric_limits<double>::quiet_NaN());
                    v_plot.push_back(std::numeric_limits<double>::quiet_NaN());
                }
                t_plot.push_back(times[i]);
                v_plot.push_back(rsrp[i]);
            }
            std::string label = "PCI " + std::to_string(pci);
            ImPlot::PlotLine(label.c_str(), t_plot.data(), v_plot.data(), (int)t_plot.size());
            ImPlot::PlotScatter(label.c_str(), t_plot.data(), v_plot.data(), (int)t_plot.size(), ImPlotSpec(ImPlotProp_MarkerSize, 2.0f, ImPlotProp_Marker, ImPlotMarker_Circle));
        }
        ImPlot::EndPlot();
    }
    ImGui::Text("Загружено серий (PCI): %d", (int)st.localData.pciTime.size());
    if (ImPlot::BeginPlot("##LocalTrajectory", ImVec2(-1, 350))) {
        ImPlot::SetupAxes("Longitude", "Latitude");
        if (!st.localData.trajLat.empty() && st.localData.trajLat.size() == st.localData.trajLon.size()) {
            const int npts = (int)st.localData.trajLat.size();

            ImPlot::PlotLine("Path", st.localData.trajLon.data(), st.localData.trajLat.data(), npts);

            std::vector<double> sx, sy;
            int step = std::max(1, npts / 2000);
            for (int i = 0; i < npts; i += step) {
                sx.push_back(st.localData.trajLon[i]); sy.push_back(st.localData.trajLat[i]);
            }
            ImPlot::PlotScatter("Points", sx.data(), sy.data(), (int)sx.size());
            double curLon = st.currentUser.location.longitude;
            double curLat = st.currentUser.location.latitude;
            ImPlot::PlotScatter("Current", &curLon, &curLat, 1, ImPlotSpec(ImPlotProp_Marker, ImPlotMarker_Circle, ImPlotProp_MarkerSize, 6.0f, ImPlotProp_MarkerFillColor, ImVec4(1,0,0,1)));
        }
        ImPlot::EndPlot();
    }
}

void TabMap(RuntimeState& st) {
    auto& ms = st.mapState;
    ImGui::BeginChild("MapControls", ImVec2(-1, 160), true);

    ImGui::Text("Данные:"); ImGui::SameLine();
    static int source = 0;
    ImGui::SetNextItemWidth(150);
    ImGui::Combo("##Source", &source, "В реальном времени\0Из JSON\0");
    ImGui::SameLine();
    ImGui::Text("PCI:"); ImGui::SameLine();
    static int selPci = -1;
    ImGui::SetNextItemWidth(100);
    if (ImGui::BeginCombo("##PCI", selPci == -1 ? "Все" : std::to_string(selPci).c_str())) {
        if (ImGui::Selectable("Все", selPci == -1)) selPci = -1;
        if (source == 1) { for (auto& [pci, _] : st.localData.pciTime) if (ImGui::Selectable(std::to_string(pci).c_str(), selPci == pci)) selPci = pci; }
        else { std::lock_guard<std::mutex> lk(st.mtx); for (auto& n : st.currentUser.mobileNetworks) if (ImGui::Selectable(std::to_string(n.pci).c_str(), selPci == n.pci)) selPci = n.pci; }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::Text("EARFCN:"); ImGui::SameLine();
    static char earfcn_buf[32] = "";
    ImGui::SetNextItemWidth(80);
    ImGui::InputText("##EARFCN", earfcn_buf, sizeof(earfcn_buf));

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    ImGui::Text("Настройки IDW:"); ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::SliderFloat("Радиус (м)", &ms.heatmapConfig.radius, 10, 300, "%.0f м");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::SliderFloat("Степень", &ms.heatmapConfig.power, 1, 4, "%.1f");
    ImGui::SameLine();
    const char* crits[] = {"RSRP","RSRQ","RSSI","SINR","Altitude"};
    int crit_idx = static_cast<int>(ms.heatmapConfig.criterion);
    ImGui::SetNextItemWidth(120);
    if (ImGui::Combo("Критерий", &crit_idx, crits, IM_ARRAYSIZE(crits))) ms.heatmapConfig.criterion = static_cast<osm14::HeatmapCriterion>(crit_idx);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    ImGui::Text("Режим:"); ImGui::SameLine();
    ImGui::RadioButton("Общая картинка", (int*)&ms.heatmapConfig.perTile, 0); ImGui::SameLine();
    ImGui::RadioButton("По тайлам OSM", (int*)&ms.heatmapConfig.perTile, 1);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    if (ImGui::Button("ГЕНЕРИРОВАТЬ ТЕПЛОВУЮ КАРТУ", ImVec2(250, 25))) {
        std::vector<HeatmapPoint> pts;
        std::string earfcn_filter = earfcn_buf;
        std::string crit_name = criterion_to_string(ms.heatmapConfig.criterion);
        
        if (source == 1) {
            std::ifstream f("drive_test_log.json");
            std::string line;
            int loaded = 0, filtered = 0;
            while (std::getline(f, line)) {
                try {
                    auto j = json::parse(line);
                    double lat = j["location"]["latitude"];
                    double lon = j["location"]["longitude"];
                    
                    for (auto& c : j["telephony"]) {
                        if (!earfcn_filter.empty()) {
                            int earfcn = c.value("earfcn", -1);
                            if (earfcn == -1 && c.contains("CellIdentityLte")) 
                                earfcn = c["CellIdentityLte"].value("EARFCN", -1);
                            if (std::to_string(earfcn) != earfcn_filter) continue;
                            filtered++;
                        }
                        
                        int pci = c.value("pci", -1);
                        if (pci == -1 && c.contains("CellIdentityLte")) 
                            pci = c["CellIdentityLte"].value("PCI", -1);
                        if (selPci != -1 && pci != selPci) continue;
                        
                        double val = -9999;
                        if (ms.heatmapConfig.criterion == HeatmapCriterion::RSRP) {
                            val = c.value("rsrp", -140.0);
                            if (val == -140 && c.contains("CellSignalStrengthLte")) 
                                val = (double)c["CellSignalStrengthLte"].value("RSRP", -140);
                        } else if (ms.heatmapConfig.criterion == HeatmapCriterion::RSRQ) {
                            val = c.value("rsrq", -30.0);
                            if (val == -30 && c.contains("CellSignalStrengthLte")) 
                                val = (double)c["CellSignalStrengthLte"].value("RSRQ", -30);
                        } else if (ms.heatmapConfig.criterion == HeatmapCriterion::RSSI) {
                            val = c.value("rssi", -140.0);
                            if (val == -140 && c.contains("CellSignalStrengthLte")) 
                                val = (double)c["CellSignalStrengthLte"].value("RSSI", -140);
                        } else if (ms.heatmapConfig.criterion == HeatmapCriterion::Altitude) {
                            val = j["location"].value("altitude", 0.0);
                        }
                        
                        if (val > -9998) {
                            pts.push_back({lat, lon, val});
                            loaded++;
                        }
                    }
                } catch (...) {}
            }
            LogInfo("[HEATMAP] Loaded " + std::to_string(loaded) + " points from JSON" + 
                   (filtered > 0 ? " (filtered " + std::to_string(filtered) + " by EARFCN)" : ""));
        } else {
            std::lock_guard<std::mutex> lk(st.mtx);
            for (auto& mp : st.heatmapPoints) {
                if (!earfcn_filter.empty() && std::to_string(mp.earfcn) != earfcn_filter) continue;
                if (selPci != -1 && mp.pci != selPci) continue;

                double val = -9999;
                if (ms.heatmapConfig.criterion == HeatmapCriterion::RSRP) val = (double)mp.rsrp;
                else if (ms.heatmapConfig.criterion == HeatmapCriterion::RSRQ) val = (double)mp.rsrq;
                else if (ms.heatmapConfig.criterion == HeatmapCriterion::RSSI) val = (double)mp.rssi;
                else if (ms.heatmapConfig.criterion == HeatmapCriterion::Altitude) val = mp.altitude;

                if (val > -9998) {
                    pts.push_back({mp.latitude, mp.longitude, val});
                }
            }
        }
        
        if (!pts.empty()) {
            ms.heatmapConfig.zoom = ms.zoom;
            ms.heatmapGen.Start(pts, ms.heatmapConfig, crit_name, earfcn_filter);
            LogInfo("[HEATMAP] Started generation: " + std::to_string(pts.size()) + " points, " + 
                   crit_name + ", radius=" + std::to_string(ms.heatmapConfig.radius) + "m");
        } else {
            LogError("[HEATMAP] No valid data points for heatmap generation!");
            ImGui::OpenPopup("Heatmap Error");
        }
    }
    
    if (ImGui::BeginPopupModal("Heatmap Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Нет данных для генерации тепловой карты!\nПроверьте:\n- Наличие точек в траектории\n- Фильтры PCI/EARFCN\n- Выбранный критерий");
        if (ImGui::Button("OK", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Показать Heatmap", &ms.showHeat);
    ImGui::SameLine();
    ImGui::SliderFloat("Alpha", &ms.heatAlpha, 0, 1);

    ImGui::Text("Зум: %d | Центр: %.5f, %.5f", ms.zoom, ms.centerLat, ms.centerLon);
    if (ImGui::Button("Центр на GPS")) {
        std::lock_guard<std::mutex> lk(st.mtx);
        ms.centerLat = st.currentUser.location.latitude;
        ms.centerLon = st.currentUser.location.longitude;
    }

    ImGui::EndChild();

    bool open = true;
    DrawMapWindow(open, st);
}

void TabTelephony(RuntimeState& st) {
    std::lock_guard<std::mutex> lk(st.mtx);
    int i = 0;
    for (auto& n : st.currentUser.mobileNetworks) {
        ImGui::PushID(i++);
        if (ImGui::CollapsingHeader((n.networkType + " PCI:" + std::to_string(n.pci)).c_str())) {
            ImGui::Text("RSRP: %d | RSRQ: %d | RSSI: %d", n.rsrp, n.rsrq, n.rssi);
            ImGui::Text("Cell ID: %s | TAC: %d", n.cellIdentity.c_str(), n.tac);
        }
        ImGui::PopID();
    }
}

void TabFilters(RuntimeState& st) {
    auto& f = GetFilterState();
    static std::string ip = "127.0.0.1";
    if (ImGui::Checkbox("GPS", &f.gps)) SendFilterCommand(ip, 5559, f);
    if (ImGui::Checkbox("LTE", &f.lte)) SendFilterCommand(ip, 5559, f);
    if (ImGui::Checkbox("NR",  &f.nr))  SendFilterCommand(ip, 5559, f);
    char buf[64]; strcpy(buf, ip.c_str());
    if (ImGui::InputText("IP", buf, 64)) ip = buf;
}
