# literent
A lite torrent client written in c++ using the libtorrent rasterbar library.
> [!NOTE]
> Literent is made for linux, it is currently not in scope to port it to windows.
# How to compile?
Requirements: `libtorrent-rasterber, glfw3, cmake, compiler of your choice`
To compile, use the included `build.sh` file (if you are on an rpm based distribution), otherwise follow the `CMakePresets.json` file
# How to use?
literent is a fairly light, GUI application.
It supports both wayland and xorg.
Usage is extremely simple, to add a torrent click the new torrent button.
# Why should i use this?
Most torrent clients run badly on old hardware due to their qt or gtk libraries but literent uses ImGUI, which is extremely light and fast.