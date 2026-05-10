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
    src/audiobuffer.h
    src/audiobuffer.cpp
    src/audiointerface.h
    src/audiointerface.cpp
    src/waveformvisual.h
    src/waveformvisual.cpp
    src/file_io.h
    src/file_io.cpp
    src/gui/mainwindow.cpp
    src/gui/mainwindow.h
    src/gui/audiowidget.h
    src/gui/audiowidget.cpp
    src/gui/settings.h
    src/gui/settings.cpp

    # ui files
    src/gui/mainwindow.ui
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
