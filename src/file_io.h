#pragma once

#include "audiobuffer.h"

#include <string>

class FileIO {
public:
	enum {
		FORMAT_WAV,
		FORMAT_OGG,
		FORMAT_MP3,
		FORMAT_M4A,
	};

	bool read(AudioBuffer& buffer, const std::string& path);
	bool write(const AudioBuffer& buffer, const std::string& path, int format);
};
