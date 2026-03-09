find_path(SNDFILE_INCLUDE_DIR sndfile.h)
find_library(SNDFILE_LIBRARY sndfile)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	SndFile
	REQUIRED_VARS SNDFILE_LIBRARY SNDFILE_INCLUDE_DIR
)

if(SndFile_FOUND AND NOT TARGET SndFile::sndfile)
	add_library(SndFile::sndfile UNKNOWN IMPORTED)

	set_target_properties(
		SndFile::sndfile
		PROPERTIES
		IMPORTED_LOCATION "${SNDFILE_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${SNDFILE_INCLUDE_DIR}"
	)
endif()

