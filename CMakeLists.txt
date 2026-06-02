cmake_minimum_required(VERSION 3.16)

project(rework_osm_step14 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

find_package(cppzmq CONFIG REQUIRED)
find_package(nlohmann_json CONFIG QUIET)
find_package(PostgreSQL REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(CURL QUIET)
find_package(Threads REQUIRED)

add_library(tp_imgui STATIC
    ${IMG_DIR}/imgui/imgui.cpp
    ${IMG_DIR}/imgui/imgui_draw.cpp
    ${IMG_DIR}/imgui/imgui_tables.cpp
    ${IMG_DIR}/imgui/imgui_widgets.cpp
    ${IMG_DIR}/imgui/backends/imgui_impl_sdl2.cpp
    ${IMG_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(tp_imgui PUBLIC
    ${IMG_DIR}/imgui
    ${IMG_DIR}/imgui/backends
)

target_link_libraries(tp_imgui PUBLIC
    SDL2::SDL2
    GLEW::GLEW
    OpenGL::GL
)

add_library(tp_implot STATIC
    ${IMG_DIR}/implot/implot.cpp
    ${IMG_DIR}/implot/implot_items.cpp
)

target_include_directories(tp_implot PUBLIC
    ${IMG_DIR}/implot
    ${IMG_DIR}/imgui
)

target_link_libraries(tp_implot PUBLIC
    tp_imgui
)

add_executable(rework_app

    src/main.cpp

    src/gui/gui_app.cpp
    src/gui/gui_tabs.cpp

    src/zmg/zmq_receiver.cpp

    src/yadro/json_parser.cpp

    src/db/pgsql_minimal.cpp

    src/curl/osm_tile_fetcher.cpp
    src/curl/osm_tile_texture.cpp

    src/ui/menu_bar.cpp
    src/ui/data_window.cpp
    src/ui/graph_window.cpp
    src/ui/map_window.cpp
    src/ui/ui_widget.cpp

    src/logging.cpp
    src/yadro/heatmap_generator.cpp
)

target_include_directories(rework_app PRIVATE
    include
    ${IMG_DIR}/imgui
    ${IMG_DIR}/implot
)

target_link_libraries(rework_app PRIVATE
    cppzmq
    PostgreSQL::PostgreSQL
    tp_imgui
    tp_implot
    Threads::Threads
)

if(CURL_FOUND)
    target_link_libraries(rework_app PRIVATE
        CURL::libcurl
    )
endif()

if(nlohmann_json_FOUND)
    target_link_libraries(rework_app PRIVATE
        nlohmann_json::nlohmann_json
    )
endif()