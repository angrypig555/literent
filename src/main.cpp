#include<iostream>
#include<thread>
#include<chrono>
#include<vector>
#include<cstdlib>
#include<fstream>

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


static char magnet_link[1024] = "";
static char download_dir[1024] = ".";

std::string filePathName;
std::string filePath;
std::string torrentFileContents;

int fileopenfailed = 0;
bool LiterentToolActive = true;

static bool open_about = false;
static bool open_credits = false;

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

int show_legal_popup = 1;

int imgui_init() {
    if (!glfwInit()) return 1;
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Literent Torrent Client", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    return 0;
}

int main() {
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
    torrent_ses.apply_settings(pack);
    while (!glfwWindowShouldClose(window)) {
        

        // \/ window logic /\ torrent code
        glfwPollEvents();


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        if (show_legal_popup == 1) {
            ImGui::OpenPopup("Warning");
        }
        if (ImGui::BeginPopupModal("Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Warning:\n"
                        "Literent is a peer to peer filesharing tool using the bittorrent protocol.\n"
                        "When you download a torrent, it's files will be shared back to people on the internet.\n"
                        "Any content you share is your responsibility.");
            if (ImGui::Button("I agree")) {
                show_legal_popup = 0;
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

        if (ImGui::BeginPopupModal("about_popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Literent \n"
                        "Lite bittorrent client\n"
                        "angrypig555\n");
            
            if (ImGui::Button("Ok")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("credits_popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Dear ImGui - Copyright (c) 2014-2026 Omar Cornut, licensed under the MIT license.\n"
                        "libtorrent-rasterbar - Copyright (c) 2003-2020, Arvid Norberg All rights reserved, licensed under the BSD-3 license.\n"
                        "ImGuiFileDialog - Copyright (c) 2018-2025 Stephane Cuillerdier (aka Aiekick), licensed under the MIT license. \n"
                        "See THIRD-PARTY-LICENSES for more information");
            if (ImGui::Button("Okay")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::Text("Enter directory to save to");
        ImGui::InputTextWithHint("##downloaddir", ".", download_dir, IM_ARRAYSIZE(download_dir));
        ImGui::Text("Please enter a magnet link");
        ImGui::InputText("##magnet", magnet_link, IM_ARRAYSIZE(magnet_link));
        if (ImGui::Button("Download")) {
            lt::add_torrent_params atp = lt::parse_magnet_uri(magnet_link);
            atp.save_path = download_dir;
            lt::torrent_handle h = torrent_ses.add_torrent(atp);
        }
        ImGui::Text("Or, open a .torrent file.");
        if (ImGui::Button("Open")) {
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
                }

            }
        }
        if (fileopenfailed == 1) {
            ImGui::Text("Could not open file");
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
                }
            
                ImGui::EndTable();
            }
        }
        if (ImGui::Button("Exit")) break; 
        
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
