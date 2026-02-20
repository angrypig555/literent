#include<iostream>
#include<thread>
#include<chrono>
#include<vector>

#include<libtorrent/session.hpp>
#include<libtorrent/session_params.hpp>
#include<libtorrent/add_torrent_params.hpp>
#include<libtorrent/torrent_handle.hpp>
#include<libtorrent/alert_types.hpp>
#include<libtorrent/magnet_uri.hpp>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "GLFW/glfw3.h"
#include<cstdlib>

static char magnet_link[1024] = "";
static char download_dir[1024] = "";

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
        ImGui::Begin("Literent");
        ImGui::Text("Enter directory to save to");
        ImGui::InputText("##downloaddir", download_dir, IM_ARRAYSIZE(download_dir));
        ImGui::Text("Please enter a magnet link");
        ImGui::InputText("##magnet", magnet_link, IM_ARRAYSIZE(magnet_link));
        if (ImGui::Button("Download")) {
            lt::add_torrent_params atp = lt::parse_magnet_uri(magnet_link);
            atp.save_path = download_dir;
            lt::torrent_handle h = torrent_ses.add_torrent(atp);
        }
        auto handles = torrent_ses.get_torrents();
        if (handles.empty()) {
            ImGui::Text("No active downloads");
        } else {
            if (ImGui::BeginTable("TorrentList", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))  {

                // 1. Setup Headers
                ImGui::TableSetupColumn("Filename");
                ImGui::TableSetupColumn("Status");
                ImGui::TableSetupColumn("Progress", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("Speed", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Remove");
                ImGui::TableHeadersRow();

                auto handles = torrent_ses.get_torrents();
                for (auto const& h : handles) {
                    lt::torrent_status s = h.status();

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", s.name.c_str());

                    ImGui::TableSetColumnIndex(1);
                    if (s.state >=0 && s.state < 8) {
                        ImGui::Text("%s", state_names[s.state]);
                    } else {
                        ImGui::Text("Unknown");
                    }

                    ImGui::TableSetColumnIndex(2);
                    ImGui::ProgressBar(s.progress, ImVec2(-FLT_MIN, 0)); 

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%.1f kB/s", s.download_rate / 1000.0f);
                    
                    ImGui::TableSetColumnIndex(4);
                    if (ImGui::Button("Remove")) {
                        if (h.is_valid()) {
                            torrent_ses.remove_torrent(h);
                        }
                    }
                }
            
                ImGui::EndTable();
            }
        }
        if (ImGui::Button("Close App")) break; 
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

/* to be implemented soon
int main(int argc, char const* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "usage: literent <magnet-link>" << std::endl;
            return 1;
        }
        lt::settings_pack p;
        p.set_int(lt::settings_pack::alert_mask, lt::alert_category::status
        | lt::alert_category::error);
        lt::session ses;

        lt::add_torrent_params atp = lt::parse_magnet_uri(argv[1]);
        atp.save_path = ".";
        lt::torrent_handle h = ses.add_torrent(atp);

        for (;;) {
            std::vector<lt::alert*> alerts;
            ses.pop_alerts(&alerts);

            for (lt::alert const* a : alerts) {
                std::cout << a->message() << std::endl;

                if (lt::alert_cast<lt::torrent_finished_alert>(a)) {
                    goto done;
                }
                if (lt::alert_cast<lt::torrent_error_alert>(a)) {
                    goto done;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        done:
        std::cout << "download finished, shutting down" << std::endl;
    }
    catch (std::exception& e) {
        std::cerr << "Error:" << e.what() << std::endl;
    }
}
*/