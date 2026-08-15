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

void Window::onClipboardSlot()
/**
 * 
 */
{
    std::thread([this]() {
        std::cout << "Pressed!" << std::endl;
        std::cout << "Text: " << local_clipboard_service_->key_scaner->get_text() << std::endl;
    }).detach();
}

Window::Window( asio::io_context &ctx ) :
    local_scanning_service_(std::make_unique<LocalFinderDevicesTypes>(ctx, "/Users/alekseypodoplelov/Documents/hyita01/etc/macvendor.db"))
    , table_(std::make_unique<DeviceTable>())
    , local_clipboard_service_(std::make_shared<LocalSharedClipboard>(ctx))
/**
 * 
 */
{   
    local_clipboard_service_->key_scaner->setCallBack([this]() {
        onClipboardSlot();
    });
}

void Window::doLocalScanningService()
/**
 * 
 */
{
    if (local_scanning_service_->is_active) {
        asio::error_code ec;
        local_scanning_service_->timer->cancel();
        local_scanning_service_->is_active = false;
        return;
    } 
    local_scanning_service_->is_active = true;
    local_scanning_service_->timer->expires_after(std::chrono::seconds(3));
    local_scanning_service_->timer->async_wait([this]( const asio::error_code ec) {
        local_scanning_service_->is_active = false;
        if (!ec) {
            std::vector<DiscoveredDevice> list_devices{local_scanning_service_->device_scaner->scanAllSubnet()};
            table_->clearTable();
            for (auto &i : list_devices) {
                table_->addRow(i);
            }
            this->doLocalScanningService();
        }
    });
}

void Window::doLocalClipboardService()
/**
 * 
 */
{
    if (local_clipboard_service_->is_active) {
        local_clipboard_service_->is_active = false;
        local_clipboard_service_->key_scaner->stopScan();
        local_clipboard_service_->multicast_bus->stopListen();
        return;
    }
    local_clipboard_service_->is_active = true;
    local_clipboard_service_->key_scaner->startScan();
    local_clipboard_service_->multicast_bus->startListen();
}

void Window::draw()
/**
 * 
 */
{
    bool flag_show = true;
    ImGui::Begin("MyApp", &flag_show, ImGuiWindowFlags_NoResize);
    ImGui::SetWindowSize(ImVec2(1024, 768), ImGuiCond_Once);

    if (ImGui::Button("Scan")) {
        doLocalScanningService();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clipboard")) {
        doLocalClipboardService();
    }
    ImGui::Separator();

    if (table_) {
        table_->drawTable();
    }

    ImGui::End();
}