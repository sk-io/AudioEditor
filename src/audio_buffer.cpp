#include "audio_buffer.h"

#include "app.h"
#include <QFileInfo>
#include <QtGlobal>
#include <qlogging.h>
#include <stdint.h>
#include <algorithm>

AudioBuffer::AudioBuffer() {}

void AudioBuffer::init(int num_channels, int sample_rate, std::vector<float>&& samples) {
    m_num_channels = num_channels;
    m_sample_rate = sample_rate;
    m_samples = std::move(samples);
    on_length_changed();
}

// TODO: refactor
bool AudioBuffer::load_from_file(const QString& path) {
	bool result = the_app.io.read(*this, path.toStdString());

    on_length_changed();
    return result;
}

void AudioBuffer::sample_amplitude(int channel, int64_t start, int64_t end, float& out_max, float& out_min) const {
    Q_ASSERT(m_sample_rate > 0);
    Q_ASSERT(m_num_channels > 0);
    Q_ASSERT(end >= start);

    if (start == end)
        end = start + 1;

    float max = -2, min = 2;
    for (int64_t i = start; i < end; i++) {
        int64_t index = i * m_num_channels + channel;
        if (index < 0 || index >= m_samples.size())
            continue;
        float sample = m_samples[index];
        if (sample > max)
            max = sample;
        if (sample < min)
            min = sample;
    }

    out_max = max;
    out_min = min;
}

bool AudioBuffer::delete_region(int64_t start, int64_t end) {
    Q_ASSERT(start < end);
    start = clamp_frame(start);
    end = clamp_frame(end);
    if (start == end)
        return false;

    start *= m_num_channels;
    end *= m_num_channels;

    m_samples.erase(m_samples.begin() + start, m_samples.begin() + end);
    on_length_changed();
    return true;
}

void AudioBuffer::insert_silence(int64_t where, int64_t num_frames) {
    int64_t num = num_frames * m_num_channels;
    for (int64_t i = 0; i < num; i++) {
        m_samples.push_back(0);
    }
    on_length_changed();
}

void AudioBuffer::normalize_region(int64_t start, int64_t end) {
    start = clamp_frame(start);
    end = clamp_frame(end);

    if (start >= end)
        return;

    float left_min = 0, left_max = 0;
    sample_amplitude(0, start, end, left_min, left_max);
    float right_min = 0, right_max = 0;
    if (m_num_channels == 2)
        sample_amplitude(1, start, end, right_min, right_max);

    float abs_max = std::max(std::abs(left_min), std::abs(left_max));
    const float min_amp = 0.0001f;
    abs_max = std::max(min_amp, abs_max);

    float amp = 1.0f / abs_max;

    amplify_region(0, start, end, amp);
    if (m_num_channels == 2)
        amplify_region(1, start, end, amp);
}

void AudioBuffer::amplify_region(int channel, int64_t start, int64_t end, float amp) {
    Q_ASSERT(start < end);
    start = clamp_frame(start);
    end = clamp_frame(end);

    for (int64_t i = start; i < end; i++) {
        int64_t index = i * m_num_channels + channel;

        // TODO: remove some of this crap
        if (index < 0 || index >= m_samples.size())
            continue;

        m_samples[index] *= amp;
    }
}

bool AudioBuffer::copy_region(int64_t start, int64_t end, AudioBuffer& to) const {
    Q_ASSERT(start < end);
    start = clamp_frame(start);
    end = clamp_frame(end);
    if (start == end)
        return false;

    start *= m_num_channels;
    end *= m_num_channels;

    to.init(m_num_channels, m_sample_rate);
    to.m_samples.insert(to.m_samples.begin(), m_samples.begin() + start, m_samples.begin() + end);
    to.on_length_changed();
    return true;
}

bool AudioBuffer::cut_region(int64_t start, int64_t end, AudioBuffer& to) {
    Q_ASSERT(start < end);
    start = clamp_frame(start);
    end = clamp_frame(end);
    if (start == end)
        return false;

    copy_region(start, end, to);
    delete_region(start, end);
    return true;
}

bool AudioBuffer::paste_from(int64_t where, const AudioBuffer& from) {
    if (where >= m_num_frames) {
        insert_silence(m_num_frames, where - m_num_frames);
    }

    where *= m_num_channels;

    m_samples.insert(m_samples.begin() + where, from.m_samples.begin(), from.m_samples.end());
    on_length_changed();
    return true;
}

void AudioBuffer::on_length_changed() {
    m_num_frames = m_samples.size() / m_num_channels;
    m_total_duration = m_num_frames / (double) m_sample_rate;
}
