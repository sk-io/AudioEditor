#include "audiobuffer.h"
#include "app.h"
#include <sndfile.h>
#include <stdint.h>
#include <algorithm>
#include <QFileInfo>
#include <QtGlobal>
#include <qlogging.h>

AudioBuffer::AudioBuffer() {}

AudioBuffer::AudioBuffer(int num_channels, int sample_rate, std::vector<float>&& samples) :
    m_num_channels(num_channels), m_sample_rate(sample_rate), m_samples(samples)
{
    on_length_changed();
}

void AudioBuffer::init(int num_channels, int sample_rate) {
    m_num_channels = num_channels;
    m_sample_rate = sample_rate;
    m_samples.clear();
    on_length_changed();
}

bool AudioBuffer::load_from_file(const QString& path) {
    SF_INFO info;
    memset(&info, 0, sizeof(info));

    std::string path_str = path.toStdString();
    SNDFILE* file = sf_open(path_str.c_str(), SFM_READ, &info);

    if (!file) {
        show_error_box("Error opening file: " + QString(sf_strerror(file)));
        return false;
    }

    Q_ASSERT(file);
    Q_ASSERT(info.channels > 0 && info.channels <= 2);

    m_sample_rate = info.samplerate;
    m_num_channels = info.channels;

    sf_count_t num_samples = sf_seek(file, 0, SF_SEEK_END) * m_num_channels;
    sf_seek(file, 0, SF_SEEK_SET);

    m_samples.clear();

    if (num_samples > 0) {
        float* temp_samples = new float[num_samples];
        sf_count_t num_read = sf_read_float(file, temp_samples, num_samples);

        if (!num_read) {
            qInfo(sf_strerror(file));
            Q_ASSERT(false);
        }

        m_samples.insert(m_samples.begin(), temp_samples, temp_samples + num_samples);
    }

    sf_close(file);
    on_length_changed();
    return true;
}

bool AudioBuffer::save_to_file(const QString& path) {
    // TODO: proper error handling
    SF_INFO info;
    memset(&info, 0, sizeof(info));

    // TODO: allow for customizing parameters like bitrate, format, etc.
    QFileInfo file_info(path);
    QString suffix = file_info.completeSuffix();
    if (suffix == "wav") {
        info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
    } else if (suffix == "mp3") {
        info.format = SF_FORMAT_MPEG_LAYER_III | SF_FORMAT_PCM_24;
    } else if (suffix == "ogg") {
        info.format = SF_FORMAT_OGG | SF_FORMAT_PCM_24;
    } else {
        Q_ASSERT(false);
    }

    info.samplerate = m_sample_rate;
    info.channels = m_num_channels;

    std::string path_str = path.toStdString();
    SNDFILE* file = sf_open(path_str.c_str(), SFM_WRITE, &info);
    Q_ASSERT(file);

    if (m_num_frames > 0) {
        sf_count_t num_written = sf_write_float(file, &m_samples[0], m_samples.size());
        Q_ASSERT(num_written == m_samples.size());
    }

    sf_close(file);
    return true;
}

// TODO: rename max/min to pos/neg?
void AudioBuffer::sample_amplitude(int channel, int64_t start, int64_t end, float& out_max, float& out_min) {
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
    start = limit_bounds(start);
    end = limit_bounds(end);
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
    start = limit_bounds(start);
    end = limit_bounds(end);

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
    start = limit_bounds(start);
    end = limit_bounds(end);

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
    start = limit_bounds(start);
    end = limit_bounds(end);
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
    start = limit_bounds(start);
    end = limit_bounds(end);
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

int64_t AudioBuffer::limit_bounds(int64_t frame_pos) const {
    return std::max(std::min(frame_pos, m_num_frames), (int64_t) 0);
}
