#pragma once

#include "ffmpeg_wrapper.h"
#include <vector>
#include <stdint.h>
#include <QString>

class AudioBuffer {
public:
    AudioBuffer();

    void init(int num_channels, int sample_rate, std::vector<float>&& samples = {});
    bool load_from_file(const QString& path);
    bool save_to_file(const QString& path);
    void sample_amplitude(int channel, int64_t start, int64_t end, float& out_max, float& out_min) const;
    bool delete_region(int64_t start, int64_t end);
    void insert_silence(int64_t where, int64_t num_frames);
    void normalize_region(int64_t start, int64_t end);
    void amplify_region(int channel, int64_t start, int64_t end, float amp);

    bool copy_region(int64_t start, int64_t end, AudioBuffer& to) const;
    bool cut_region(int64_t start, int64_t end, AudioBuffer& to);
    bool paste_from(int64_t where, const AudioBuffer& from);

    int64_t get_num_frames() const { return m_num_frames; }
    int get_num_channels() const { return m_num_channels; }
    int get_sample_rate() const { return m_sample_rate; }
    double get_duration() const { return m_total_duration; }
    int64_t get_frame(double time) const { return (int64_t) (time * m_sample_rate); }
    double get_time(int64_t frame_pos) const { return frame_pos / (double)m_sample_rate; }
    bool is_stereo() const { return m_num_channels == 2; }
    const std::vector<float>& get_samples() const { return m_samples; }
	float single_sample(int64_t frame, int channel) const {
		return m_samples[clamp_frame(frame) * 2 + channel];
	}

    float* get_raw_pointer() {
        return m_samples.size() == 0 ? nullptr : &m_samples[0];
    }

	int64_t clamp_frame(int64_t frame) const {
		return std::max((int64_t) 0, std::min(m_num_frames - 1, frame));
	}

	double clamp_time(double time) const {
		return std::max(0.0, std::min(m_total_duration, time));
	}

private:
    void on_length_changed();

private:
    std::vector<float> m_samples;
	int m_sample_format = AV_SAMPLE_FMT_FLT;
    int64_t m_num_frames = 0;
    int m_sample_rate = -1;
    int m_num_channels = -1;
    double m_total_duration = -1;
};
