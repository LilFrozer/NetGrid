#include "Window.h"
#include "imgui.h"

void DeviceTable::addRow(const DiscoveredDevice& device) 
/**
 * 
 */
{
    std::lock_guard<std::mutex> lock(mutex_);
    data_.push_back(device);
}

void DeviceTable::clearTable() 
/**
 * 
 */
{
    std::lock_guard<std::mutex> lock(mutex_);
    data_.clear();
}

size_t DeviceTable::getRowCount() const 
/**
 * 
 */
{
    std::lock_guard<std::mutex> lock(mutex_);
    return data_.size();
}

void DeviceTable::drawTable() 
/**
 * 
 */
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (ImGui::BeginTable("DevicesTable", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                          ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("IP", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("Hostname", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("MAC", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Vendor", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const auto& dev : data_) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", dev.addr.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", dev.hostname.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", dev.mac.c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", dev.vendor.c_str());
        }

        ImGui::EndTable();
    }
}

Window::Window( asio::io_context &ctx ) :
    local_finder_(std::make_unique<LocalFinderDevicesTypes>(ctx, "/Users/alekseypodoplelov/Documents/hyita01/etc/macvendor.db"))
    , table_(std::make_unique<DeviceTable>())
    // , multicast_bus_(std::make_unique<MulticastBus>(ctx))
/**
 * 
 */
{   
}

void Window::startScan()
/**
 * 
 */
{
    if (local_finder_->timer_active) {
        asio::error_code ec;
        local_finder_->timer->cancel();
        local_finder_->timer_active = false;
        return;
    } 
    local_finder_->timer_active = true;
    local_finder_->timer->expires_after(std::chrono::seconds(3));
    local_finder_->timer->async_wait([this]( const asio::error_code ec) {
        local_finder_->timer_active = false;
        if (!ec) {
            std::cout << "scan!" << std::endl;
            std::vector<DiscoveredDevice> list_devices{local_finder_->network_scaner->scanAllSubnet()};
            for (auto &i : list_devices) {
                table_->clearTable();
                table_->addRow(i);
                // std::cout << "find! -> " << i.addr << "/" << i.hostname << "/" << i.mac << "/" << i.vendor << std::endl;
            }
            this->startScan();
        }
    });
}

void Window::draw()
/**
 * 
 */
{
    bool flag_show = true;
    ImGui::Begin("MyApp", &flag_show, ImGuiWindowFlags_NoResize);
    ImGui::SetWindowSize(ImVec2(1024, 768), ImGuiCond_Once);

    // ---- Кнопки ----
    if (ImGui::Button("Scan")) {
        startScan();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        table_->clearTable();
    }
    ImGui::Separator();

    // ---- Статус ----
    // if (scan_manager_->isScanning()) {
    //     ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Scanning...");
    // } else {
    //     ImGui::Text("Ready");
    // }
    // ImGui::Separator();

    // ---- Таблица ----
    if (table_) {
        table_->drawTable();
    }

    ImGui::End();
}