sudo apt install ninja pkg-config libcairo2-dev libegl1-mesa-dev libgbm-dev libgtkmm-3.0-dev 
libxcb-dri3-dev libxcb-present-dev libxcb-render-util0-dev libxcb-shm0-dev libxcb-xinput-dev
sudo python3 -m pip install meson and add to PATH
git submodule update --recursive --init

before running make sure you are using our custome libdrm
export LD_PRELOAD="/home/yevhen/Develop/HomePet/linuxCompositors/build/libdrm-prefix/lib/libdrm.so.2 
/home/yevhen/Develop/HomePet/linuxCompositors/build/wayland-scanner-prefix/lib/x86_64-linux-gnu/libwayland-client.so.0 
/home/yevhen/Develop/HomePet/linuxCompositors/build/xkbcommon-prefix/lib/libxkbcommon.so.0 
/home/yevhen/Develop/HomePet/linuxCompositors/build/pixman-prefix/lib/libpixman-1.so.0"
ldd ./wayland-server
