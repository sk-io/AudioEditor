find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
find_library(PORTAUDIO_LIBRARY portaudio)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	PortAudio
	REQUIRED_VARS PORTAUDIO_LIBRARY PORTAUDIO_INCLUDE_DIR
)

if(PortAudio_FOUND AND NOT TARGET PortAudio::portaudio)
	add_library(PortAudio::portaudio UNKNOWN IMPORTED)

	set_target_properties(
		PortAudio::portaudio
		PROPERTIES
		IMPORTED_LOCATION "${PORTAUDIO_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${PORTAUDIO_INCLUDE_DIR}"
	)
endif()

