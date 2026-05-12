cmake_minimum_required(VERSION 3.16)

project(AudioEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# add_compile_options(-fno-omit-frame-pointer)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(PortAudio REQUIRED)
find_package(FFmpeg REQUIRED)

add_executable(AudioEditor
    # sources
    src/main.cpp
    src/app.h
    src/app.cpp
    src/audio_buffer.h
    src/audio_buffer.cpp
    src/audio_interface.h
    src/audio_interface.cpp
    src/waveform_cache.h
    src/waveform_cache.cpp
    src/file_io.h
    src/file_io.cpp
    src/gui/main_window.cpp
    src/gui/main_window.h
    src/gui/audio_widget.h
    src/gui/audio_widget.cpp
    src/gui/settings.h
    src/gui/settings.cpp

    # ui files
    src/gui/main_window.ui
    src/gui/settings.ui

    # resource files
    resources.qrc
)

target_link_libraries(AudioEditor
    PRIVATE
        Qt::Widgets
        PortAudio::portaudio
		FFmpeg::avformat
		FFmpeg::avcodec
		FFmpeg::avutil
		FFmpeg::swresample
)
