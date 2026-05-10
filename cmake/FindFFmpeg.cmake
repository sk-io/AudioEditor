# FindFFmpeg.cmake
#
# Finds:
#   libavformat
#   libavcodec
#   libavutil
#   libswresample
#
# Provides:
#
#   FFmpeg_FOUND
#
#   FFmpeg_INCLUDE_DIRS
#   FFmpeg_LIBRARIES
#
# Imported targets:
#
#   FFmpeg::avformat
#   FFmpeg::avcodec
#   FFmpeg::avutil
#   FFmpeg::swresample

include(FindPackageHandleStandardArgs)

find_path(FFmpeg_INCLUDE_DIR
    NAMES libavformat/avformat.h
)

find_library(FFmpeg_avformat_LIBRARY
    NAMES avformat
)

find_library(FFmpeg_avcodec_LIBRARY
    NAMES avcodec
)

find_library(FFmpeg_avutil_LIBRARY
    NAMES avutil
)

find_library(FFmpeg_swresample_LIBRARY
    NAMES swresample
)

set(FFmpeg_INCLUDE_DIRS
    ${FFmpeg_INCLUDE_DIR}
)

set(FFmpeg_LIBRARIES
    ${FFmpeg_avformat_LIBRARY}
    ${FFmpeg_avcodec_LIBRARY}
    ${FFmpeg_avutil_LIBRARY}
    ${FFmpeg_swresample_LIBRARY}
)

find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS
        FFmpeg_INCLUDE_DIR
        FFmpeg_avformat_LIBRARY
        FFmpeg_avcodec_LIBRARY
        FFmpeg_avutil_LIBRARY
        FFmpeg_swresample_LIBRARY
)

if(FFmpeg_FOUND)

    add_library(FFmpeg::avformat UNKNOWN IMPORTED)
    set_target_properties(FFmpeg::avformat PROPERTIES
        IMPORTED_LOCATION "${FFmpeg_avformat_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_INCLUDE_DIR}"
    )

    add_library(FFmpeg::avcodec UNKNOWN IMPORTED)
    set_target_properties(FFmpeg::avcodec PROPERTIES
        IMPORTED_LOCATION "${FFmpeg_avcodec_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_INCLUDE_DIR}"
    )

    add_library(FFmpeg::avutil UNKNOWN IMPORTED)
    set_target_properties(FFmpeg::avutil PROPERTIES
        IMPORTED_LOCATION "${FFmpeg_avutil_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_INCLUDE_DIR}"
    )

    add_library(FFmpeg::swresample UNKNOWN IMPORTED)
    set_target_properties(FFmpeg::swresample PROPERTIES
        IMPORTED_LOCATION "${FFmpeg_swresample_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_INCLUDE_DIR}"
    )

endif()
