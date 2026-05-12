#pragma once

#include "audio_buffer.h"

#include <string>

class FileIO {
public:
	bool read(AudioBuffer& buffer, const std::string& path);
	bool write(const AudioBuffer& buffer, const std::string& path, int format);
};
