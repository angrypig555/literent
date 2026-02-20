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
