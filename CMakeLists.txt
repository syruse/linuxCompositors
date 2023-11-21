cmake_minimum_required(VERSION 3.16)
include(ExternalProject)
set(APP_NAME wayland-server)
project(${APP_NAME} VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_SUPPRESS_REGENERATION true)

if( NOT CMAKE_BUILD_TYPE )
   set( CMAKE_BUILD_TYPE Release )
endif()

if(CMAKE_BUILD_TYPE MATCHES Release)
   message("${CMAKE_CXX_FLAGS_RELEASE}")
else()
   message("${CMAKE_CXX_FLAGS_DEBUG}")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(MESON_BUILD_TYPE --buildtype debug)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
	set(MESON_BUILD_TYPE --buildtype release)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
	set(MESON_BUILD_TYPE --buildtype debugoptimized)
endif()

find_package(EGL REQUIRED)

# When building for Debian like system, Meson uses multiarch libdir (e.g.
# 'lib/x86_64-linux-gnu'), and when cross-compiling libdir is just 'lib'.
# Therefore explicitly set libdir Meson built-in option to have common
# location.
ExternalProject_Add(pixman
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/pixman
    CONFIGURE_COMMAND
        meson setup
        ${MESON_BUILD_TYPE}
        --prefix <INSTALL_DIR>
        -D libdir=lib
        --reconfigure
        # pixman options
        -D loongson-mmi=disabled
        -D mmx=disabled
        -D sse2=disabled
        -D ssse3=disabled
        -D vmx=disabled
        -D arm-simd=disabled
        -D neon=disabled
        -D a64-neon=disabled
        -D iwmmxt=disabled
        -D mips-dspr2=disabled
        -D tests=disabled
        <SOURCE_DIR>
    BUILD_COMMAND meson compile
    INSTALL_COMMAND meson install
    BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libpixman-1.so
)
ExternalProject_Add(wayland-scanner
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/wayland
    CONFIGURE_COMMAND
        meson setup
        --prefix <INSTALL_DIR>
        --pkg-config-path=/usr/lib/${CMAKE_CXX_LIBRARY_ARCHITECTURE}/pkgconfig
        --reconfigure
        # wayland-scanner options
        -D tests=false
        -D documentation=false
        -D dtd_validation=false
        <SOURCE_DIR>
    BUILD_COMMAND meson compile
    INSTALL_COMMAND meson install
)
ExternalProject_Add(wayland
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/wayland
    CONFIGURE_COMMAND
        meson setup
        --prefix <INSTALL_DIR>
		--pkg-config-path ${CMAKE_BINARY_DIR}/wayland-scanner-prefix/lib/${CMAKE_CXX_LIBRARY_ARCHITECTURE}/pkgconfig 
        -D libdir=lib
        --reconfigure
        # wayland options
        -D scanner=false
        -D tests=false
        -D documentation=false
        -D dtd_validation=false
        <SOURCE_DIR>
    BUILD_COMMAND meson compile
    INSTALL_COMMAND meson install
    DEPENDS wayland-scanner
    BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libwayland-server.so
)
ExternalProject_Add(libdrm
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/libdrm
    CONFIGURE_COMMAND
        meson setup
        --prefix <INSTALL_DIR>
        -D libdir=lib
        # libdrm options
        -D amdgpu=disabled
        -D etnaviv=disabled
        -D freedreno=disabled
        -D intel=disabled
        -D nouveau=disabled
        -D radeon=disabled
        -D vc4=disabled
        -D tests=false
        <SOURCE_DIR>
    BUILD_COMMAND meson compile
    INSTALL_COMMAND meson install
    BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libdrm.so
)

ExternalProject_Add(wayland-protocols
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/wayland-protocols
    CONFIGURE_COMMAND
        meson setup
        ${MESON_BUILD_TYPE}
        --prefix <INSTALL_DIR>
        --reconfigure
        # wayland-protocols options
        -D tests=false
        <SOURCE_DIR>
    BUILD_COMMAND meson compile
    INSTALL_COMMAND meson install
)

ExternalProject_Add(xkbcommon
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/xkbcommon
    CONFIGURE_COMMAND
        meson setup
        ${MESON_BUILD_TYPE}
        --prefix <INSTALL_DIR>
        -D libdir=lib
        --reconfigure
        # xkbcommon options
        -D enable-tools=false
        -D enable-x11=true
        -D enable-docs=false
        -D enable-wayland=false
        -D enable-xkbregistry=false
        <SOURCE_DIR>
    BUILD_COMMAND meson compile
    INSTALL_COMMAND meson install
    BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libxkbcommon.so.0
)
set(WLROOTS_PKG_CONFIG_PATH
    ${PKG_CONFIG_PATH}
    ${CMAKE_BINARY_DIR}/wayland-scanner-prefix/lib/${CMAKE_CXX_LIBRARY_ARCHITECTURE}/pkgconfig
    ${CMAKE_BINARY_DIR}/libdrm-prefix/lib/pkgconfig
    ${CMAKE_BINARY_DIR}/pixman-prefix/lib/pkgconfig
    ${CMAKE_BINARY_DIR}/wayland-prefix/lib/pkgconfig
    ${CMAKE_BINARY_DIR}/wayland-protocols-prefix/share/pkgconfig
    ${CMAKE_BINARY_DIR}/xkbcommon-prefix/lib/pkgconfig
)

list(JOIN WLROOTS_PKG_CONFIG_PATH , WLROOTS_PKG_CONFIG_PATH)
ExternalProject_Add(wlroots
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/wlroots
    CONFIGURE_COMMAND
        meson setup
        ${MESON_BUILD_TYPE}
        --prefix <INSTALL_DIR>
        --pkg-config-path ${WLROOTS_PKG_CONFIG_PATH}
        -D libdir=lib
        --reconfigure
        # wlroots options
        -D xcb-errors=disabled
        -D xwayland=disabled
        -D examples=false
        -D renderers=gles2
        -D backends=x11
        -D allocators=gbm
        -D session=auto
        ${CMAKE_SOURCE_DIR}/thirdparty/wlroots
    BUILD_COMMAND meson compile
    INSTALL_COMMAND meson install
    DEPENDS pixman wayland libdrm wayland-protocols xkbcommon
    BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libwlroots.so
)

file(GLOB SRCS ${PROJECT_SOURCE_DIR}/**.cpp)

add_executable(${APP_NAME} main.c ${SRCS})
target_compile_definitions(${APP_NAME} PRIVATE WLR_USE_UNSTABLE)
add_dependencies(${APP_NAME} wlroots)

target_include_directories(
	${APP_NAME}
	PRIVATE
	${PROJECT_SOURCE_DIR}
	${EGL_INCLUDE_DIRS}
	${CMAKE_BINARY_DIR}/wayland-prefix/include
	thirdparty/wlroots/include/
	thirdparty/xkbcommon/include
	${CMAKE_BINARY_DIR}/pixman-prefix/include/pixman-1
	${CMAKE_BINARY_DIR}/wlroots-prefix/src/wlroots-build/protocol
)

target_link_libraries(
	${APP_NAME}
	${EGL_LIBRARIES}
	${CMAKE_BINARY_DIR}/wlroots-prefix/lib/libwlroots.so
	${CMAKE_BINARY_DIR}/wayland-prefix/lib/libwayland-server.so
	${CMAKE_BINARY_DIR}/libdrm-prefix/lib/libdrm.so
)
