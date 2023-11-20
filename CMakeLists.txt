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

find_package(PkgConfig REQUIRED)
pkg_check_modules(PIXMAN REQUIRED pixman-1)

if(PIXMAN_FOUND)
  message("PIXMAN_VERSION ${PIXMAN_VERSION} ${PIXMAN_INCLUDE_DIRS} ${PIXMAN_LIBRARIES}")
  include_directories(${PIXMAN_INCLUDE_DIRS})
endif()

find_package(Wayland REQUIRED)
find_package(EGL REQUIRED)

file(GLOB SRCS ${PROJECT_SOURCE_DIR}/**.cpp)

add_executable(${APP_NAME} main.c ${SRCS})

target_compile_definitions(${APP_NAME} PRIVATE WLR_USE_UNSTABLE)

target_include_directories(
	${APP_NAME}
	PRIVATE
	${PROJECT_SOURCE_DIR}
	${WAYLAND_INCLUDE_DIRS}
	${EGL_INCLUDE_DIRS}
)

target_link_libraries(
	wayland-server
	${WAYLAND_LIBRARIES}
	${EGL_LIBRARIES}
)