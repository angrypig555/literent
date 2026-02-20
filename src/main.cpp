#include<iostream>
#include<thread>
#include<chrono>
#include<vector>
#include<cstdlib>
#include<fstream>
#include<stdlib.h>
#include<filesystem>

#include<libtorrent/session.hpp>
#include<libtorrent/session_params.hpp>
#include<libtorrent/add_torrent_params.hpp>
#include<libtorrent/torrent_handle.hpp>
#include<libtorrent/alert_types.hpp>
#include<libtorrent/magnet_uri.hpp>
#include<libtorrent/torrent_info.hpp>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "imguifd/ImGuiFileDialog.h"

#include "GLFW/glfw3.h"

#include"toml++/toml.hpp"

static char magnet_link[1024] = "";
static char download_dir[1024] = "";

std::string filePathName;
std::string filePath;
std::string torrentFileContents;

int fileopenfailed = 0;
bool LiterentToolActive = true;

static bool open_about = false;
static bool open_credits = false;
static bool open_settings = false;
static bool open_download = false;

const char* state_names[] = {
    "Queued",                     // 0
    "Checking",                   // 1
    "Downloading Metadata",       // 2
    "Downloading",                // 3
    "Finished",                   // 4
    "Seeding",                    // 5
    "Allocating",                 // 6
    "Checking Fastresume"         // 7
};

int show_legal_popup;

std::string homedir;
std::string homedir_downloads;
std::string homeDirectory;

int attempt_counter;

std::string conf_contents;
static bool has_tried_to_open_popup = false;

static bool pex_enabled;
static bool dht_enabled;
static bool lsd_enabled;
static bool upnp_enabled;
static bool natpmp_enabled;


const char* get_home_dir() {
    homedir = getenv("HOME");
    return homedir.c_str();
}

int read_config() {
    homeDirectory = get_home_dir();
    std::string conf_folder =  homeDirectory + "/.config/";
    std::string literent_conf_folder_string = conf_folder + "literent/";
    std::string config_toml = literent_conf_folder_string + "config.toml";
    reread_config:
    std::filesystem::path literent_conf_folder = std::string(literent_conf_folder_string);
    std::filesystem::path literent_conf_toml_path = config_toml;
    bool conf_folder_exists = std::filesystem::exists(literent_conf_folder);
    if (conf_folder_exists == true) {
        bool conf_toml_exists = std::filesystem::exists(literent_conf_toml_path);
        if (conf_toml_exists == true) {
                auto conf_raw = toml::parse_file(config_toml);
                show_legal_popup = conf_raw["show_legal_popup"].value<int>().value_or(1);
                pex_enabled = conf_raw["pex_enabled"].value<bool>().value_or(true);
                dht_enabled = conf_raw["dht_enabled"].value<bool>().value_or(true);
                lsd_enabled = conf_raw["lsd_enabled"].value<bool>().value_or(true);
                upnp_enabled = conf_raw["upnp_enabled"].value<bool>().value_or(false);
                natpmp_enabled = conf_raw["natpmp_enabled"].value<bool>().value_or(false);
        } else {
            std::ofstream toml_file(config_toml);
            toml_file << "show_legal_popup = 1\n" << "pex_enabled = true\n" << "dht_enabled = true\n" << "lsd_enabled = true\n" << "upnp_enabled = false\n" << "natpmp_enabled = false\n";
            toml_file.close();
            goto reread_config;
        }
    } else {
        if (attempt_counter >= 3) {
            return 1;
        }
        std::filesystem::create_directory(literent_conf_folder);
        attempt_counter += 1;
        goto reread_config;
    }
    return 0;
}

int write_config(auto value, std::string value_name) {
    homeDirectory = get_home_dir();
    std::string conf_folder =  homeDirectory + "/.config/";
    std::string literent_conf_folder_string = conf_folder + "literent/";
    std::string config_toml = literent_conf_folder_string + "config.toml";
    auto config_raw = toml::parse_file(config_toml);

    config_raw.insert_or_assign(value_name, value);
    std::ofstream config_file(config_toml);
    config_file << config_raw << "\n";
    config_file.close();
    return 0;
}

int main() {
    show_legal_popup = 1;
    read_config();
    if (!glfwInit()) return 1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Literent", NULL, NULL);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); 

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    ImGuiIO& io = ImGui::GetIO();

    lt::session torrent_ses;
    lt::settings_pack pack;
    pack.set_str(lt::settings_pack::user_agent, "Literent/0.1.0");
    pack.set_bool(lt::settings_pack::enable_dht, dht_enabled);
    pack.set_bool(lt::settings_pack::enable_lsd, lsd_enabled);
    pack.set_bool(lt::settings_pack::enable_upnp, upnp_enabled);
    pack.set_bool(lt::settings_pack::enable_natpmp, natpmp_enabled);
    torrent_ses.apply_settings(pack);
    homeDirectory = get_home_dir();
    homedir_downloads = homeDirectory + "/Downloads/";
    snprintf(download_dir, sizeof(download_dir), "%s", homedir_downloads.c_str());
    while (!glfwWindowShouldClose(window)) {
        

        // \/ window logic /\ torrent code
        glfwPollEvents();


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if (!has_tried_to_open_popup) {
            if (show_legal_popup == 1) {
                ImGui::OpenPopup("Warning");
            }
            has_tried_to_open_popup = true;
        }
        if (ImGui::BeginPopupModal("Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Warning:\n"
                        "Literent is a peer to peer filesharing tool using the bittorrent protocol.\n"
                        "When you download a torrent, it's files will be shared back to people on the internet.\n"
                        "Any content you share is your responsibility.");
            if (ImGui::Button("I agree")) {
                show_legal_popup = 0;
                write_config(0, "show_legal_popup");
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Exit")) {
                goto exit;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }


        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->WorkPos);
        ImGui::SetNextWindowSize(ImGui::GetMainViewport()->WorkSize);
        ImGui::Begin("Literent", &LiterentToolActive, ImGuiWindowFlags_MenuBar);
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("About")) {
                if (ImGui::MenuItem("About")) {
                    open_about = true;
                }
                if (ImGui::MenuItem("Credits")) {
                    open_credits = true;
                }
                if (ImGui::MenuItem("Exit")) {
                    break;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings")) {
                if (ImGui::MenuItem("Settings")) {
                    open_settings = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("New")) {
                if (ImGui::MenuItem("New download")) {
                    open_download = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (open_about == true) {
            ImGui::OpenPopup("about_popup");
            open_about = false;
        }
        if (open_credits == true) {
            ImGui::OpenPopup("credits_popup");
            open_credits = false;
        }

        if (open_settings == true) {
            ImGui::OpenPopup("settings_popup");
            open_settings = false;
        }

        if (open_download == true) {
            ImGui::OpenPopup("new_download");
            open_download = false;
        }

        if (ImGui::BeginPopupModal("about_popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Literent \n"
                        "Lite bittorrent client\n"
                        "Copyright (c) 2026 angrypig555\n"
                        "Licensed under the MIT License\n"
                        "See LICENSE for more information\n");
            
            if (ImGui::Button("Ok")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("credits_popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Dear ImGui - Copyright (c) 2014-2026 Omar Cornut, licensed under the MIT license.\n"
                        "libtorrent-rasterbar - Copyright (c) 2003-2020, Arvid Norberg All rights reserved, licensed under the BSD-3 license.\n"
                        "ImGuiFileDialog - Copyright (c) 2018-2025 Stephane Cuillerdier (aka Aiekick), licensed under the MIT license. \n"
                        "TOML++ - Copyright (c) Mark Gillard <mark.gillard@outlook.com.au>, licensed under the MIT license\n"
                        "See THIRD-PARTY-LICENSES for more information");
            if (ImGui::Button("Okay")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("settings_popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Configuration is saved to ~/.config/literent/config.toml");
            if (ImGui::Checkbox("DHT (Distributed Hash Table)", &dht_enabled)) {
                lt::settings_pack p;
                p.set_bool(lt::settings_pack::enable_dht, dht_enabled);
                torrent_ses.apply_settings(p);
                write_config(dht_enabled, "dht_enabled");
            }
            if (ImGui::Checkbox("LSD (Local Service Discovery)", &lsd_enabled)) {
                lt::settings_pack p;
                p.set_bool(lt::settings_pack::enable_lsd, lsd_enabled);
                torrent_ses.apply_settings(p);
                write_config(lsd_enabled, "dht_enabled");
            }
            if (ImGui::Checkbox("UPNP (Universal Plug n Play)", &upnp_enabled)) {
                lt::settings_pack p;
                p.set_bool(lt::settings_pack::enable_upnp, upnp_enabled);
                torrent_ses.apply_settings(p);
                write_config(upnp_enabled, "upnp_enabled");
            }
            if (ImGui::Checkbox("NAT-PMP/PCP", &natpmp_enabled)) {
                lt::settings_pack p;
                p.set_bool(lt::settings_pack::enable_natpmp, natpmp_enabled);
                torrent_ses.apply_settings(p);
                write_config(natpmp_enabled, "natpmp_enabled");
            }
            if (ImGui::Button("Return")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("new_download", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("New download wizard");
            ImGui::Text("Please enter download directory");
            ImGui::InputTextWithHint("##downloaddir", homedir_downloads.c_str(), download_dir, IM_ARRAYSIZE(download_dir));
            const char* items[] = { "Torrent file", "Magnet link"};
            static int selected_item = 0;
            ImGui::Combo("Select a type", &selected_item, items, IM_ARRAYSIZE(items));
            if (selected_item == 0) {
                if (ImGui::Button("Open .torrent file")) {
                    IGFD::FileDialogConfig config;
                    config.path = ".";
                    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".torrent", config);
                }
                if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
                    if (ImGuiFileDialog::Instance()->IsOk()) {
                        filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                        filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                
                    }
                    ImGuiFileDialog::Instance()->Close();
                }
                if (!filePathName.empty()) {
                    std::string text_to_print = filePathName + " Loaded";
                    ImGui::Text(text_to_print.c_str());
                    if (ImGui::Button("Download .torrent")) {
                        std::ifstream file(filePathName);
                        if (!file.is_open()) { // checking if the file is openable
                            std::cerr << "failed to open file: " << filePath << std::endl;
                            fileopenfailed = 1;
                            break;
                        }
                        file.close();
                        lt::error_code ercode;

                        auto info = std::make_shared<lt::torrent_info>(filePathName, ercode);

                        if (ercode) {
                            std::cerr << "libtorrent could not open the torrent file" << std::endl;
                            fileopenfailed = 1;
                        } else {
                            lt::add_torrent_params atp;
                            atp.ti = info;
                            atp.save_path = download_dir;

                            torrent_ses.add_torrent(atp);
                            fileopenfailed = 0;
                            filePathName.clear();
                            ImGui::CloseCurrentPopup();
                        }
                        
                    }
                }
                if (fileopenfailed == 1) {
                    ImGui::Text("Could not open file");
                }
            } else if (selected_item == 1) {
                ImGui::Text("Please enter a magnet link");
                ImGui::InputText("##magnet", magnet_link, IM_ARRAYSIZE(magnet_link));
                if (ImGui::Button("Download")) {
                    lt::add_torrent_params atp = lt::parse_magnet_uri(magnet_link);
                    atp.save_path = download_dir;
                    lt::torrent_handle h = torrent_ses.add_torrent(atp);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }
        
        
        
        
        auto handles = torrent_ses.get_torrents();
        if (handles.empty()) {
            ImGui::Text("No active downloads");
        } else {
            if (ImGui::BeginTable("TorrentList", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))  {

                // 1. Setup Headers
                ImGui::TableSetupColumn("Filename");
                ImGui::TableSetupColumn("Peers");
                ImGui::TableSetupColumn("Seeds");
                ImGui::TableSetupColumn("Status");
                ImGui::TableSetupColumn("Progress", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("Speed", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Remove");
                ImGui::TableSetupColumn("Pause");
                ImGui::TableHeadersRow();

                auto handles = torrent_ses.get_torrents();
                for (auto const& h : handles) {
                    lt::torrent_status s = h.status();

                    ImGui::TableNextRow();
                    ImGui::PushID(h.id());

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", s.name.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", s.num_peers);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%d", s.num_seeds);

                    ImGui::TableSetColumnIndex(3);
                    if (s.state >=0 && s.state < 8) {
                        ImGui::Text("%s", state_names[s.state]);
                    } else {
                        ImGui::Text("Unknown");
                    }

                    ImGui::TableSetColumnIndex(4);
                    ImGui::ProgressBar(s.progress, ImVec2(-FLT_MIN, 0)); 

                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("%.1f kB/s", s.download_rate / 1000.0f);
                    
                    ImGui::TableSetColumnIndex(6);
                    if (ImGui::Button("Remove")) {
                        if (h.is_valid()) {
                            torrent_ses.remove_torrent(h);
                        }
                    }

                    ImGui::TableSetColumnIndex(7);
                    if (s.flags & lt::torrent_flags::paused) {
                        if (ImGui::Button("Resume")) {
                            if (h.is_valid()) {
                                h.resume();
                            }
                        }
                    } else {
                        if (ImGui::Button("Pause")) {
                            if (h.is_valid()) {
                                h.pause();
                            }
                        }
                    }

                    ImGui::PopID();
                }
            
                ImGui::EndTable();
            }
        }
        if (ImGui::Button("New download")) {
            open_download = true;
        }
        
        ImGui::End();
        ImGui::PopStyleVar(3);
        ImGui::Render();
        
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }
    exit:
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}
