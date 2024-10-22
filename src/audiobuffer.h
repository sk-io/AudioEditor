#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H

#include <vector>
#include <stdint.h>
#include <QString>

class AudioBuffer {
public:
    AudioBuffer();
    AudioBuffer(int num_channels, int sample_rate, std::vector<float>&& samples);

    void init(int num_channels, int sample_rate);
    void init_from_samples(int num_channels, int sample_rate, std::vector<float> samples);
    void load_from_file(const QString& path);
    void save_to_file(const QString& path);
    void sample_amplitude(int channel, double start_time, double duration, float& out_max, float& out_min);
    bool delete_region(uint64_t start, uint64_t end);

    bool copy_region(uint64_t start, uint64_t end, AudioBuffer& to) const;
    bool cut_region(uint64_t start, uint64_t end, AudioBuffer& to);
    bool paste_from(uint64_t where, const AudioBuffer& from);

    uint64_t get_num_frames() const { return m_num_frames; };
    int get_num_channels() const { return m_num_channels; }
    int get_sample_rate() const { return m_sample_rate; }
    double get_duration() const { return m_total_duration; }
    uint64_t get_frame(double time) const { return (uint64_t) (time * m_sample_rate); }
    double get_time(uint64_t frame_pos) const { return frame_pos / (double)m_sample_rate; }
    bool is_stereo() const { return m_num_channels == 2; };

    float* get_raw_pointer() {
        return m_samples.size() == 0 ? nullptr : &m_samples[0];
    }

    const std::vector<float>& get_samples() const {
        return m_samples;
    }
private:
    void on_length_changed();
    uint64_t limit_bounds(uint64_t frame_pos) const;

private:
    std::vector<float> m_samples; // stereo has 2 floats per sample, interleaved
    uint64_t m_num_frames = 0;
    int m_sample_rate = -1;
    int m_num_channels = -1;
    double m_total_duration = -1;
};

#endif // AUDIOBUFFER_H
