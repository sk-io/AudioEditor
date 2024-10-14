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
    bool delete_region(int64_t start, int64_t end);

    bool copy_region(int64_t start, int64_t end, AudioBuffer& to) const;
    bool cut_region(int64_t start, int64_t end, AudioBuffer& to);
    bool paste_from(int64_t where, const AudioBuffer& from);

    int64_t get_num_samples() const { return m_num_samples; };
    int get_num_channels() const { return m_num_channels; }
    int get_sample_rate() const { return m_sample_rate; }
    double get_duration() const { return m_total_duration; }
    int64_t get_sample_index(double time) const { return (int64_t) (time * m_sample_rate); }

private:
    std::vector<float> m_samples; // stereo has 2 floats per sample, interleaved
    int64_t m_num_samples = 0;
    int m_sample_rate = -1;
    int m_num_channels = -1;
    double m_total_duration = -1;

    void on_length_changed();
    int64_t limit_bounds(int64_t sample_index) const;
};

#endif // AUDIOBUFFER_H
