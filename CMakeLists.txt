cmake_minimum_required(VERSION 3.16)

project(AudioEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(SndFile REQUIRED)
find_package(portaudio REQUIRED)

add_executable(AudioEditor
    # sources
    src/main.cpp
    src/app.h
    src/app.cpp
    src/audiobuffer.h
    src/audiobuffer.cpp
    src/audiointerface.h
    src/audiointerface.cpp
    src/gui/mainwindow.cpp
    src/gui/mainwindow.h
    src/gui/audiowidget.h
    src/gui/audiowidget.cpp

    # ui files
    src/gui/mainwindow.ui
    src/gui/settings.h src/gui/settings.cpp src/gui/settings.ui
)

target_link_libraries(AudioEditor
	PRIVATE
		Qt::Widgets
		SndFile::sndfile
		portaudio
)
