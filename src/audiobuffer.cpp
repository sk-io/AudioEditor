#include "audiobuffer.h"
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

void AudioBuffer::load_from_file(const QString& path) {
    // TODO: proper error handling

    SF_INFO info;
    memset(&info, 0, sizeof(info));

    std::string path_str = path.toStdString();
    SNDFILE* file = sf_open(path_str.c_str(), SFM_READ, &info);

    if (!file) {
        qDebug() << "ERROR: " << sf_strerror(file);
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
}

void AudioBuffer::save_to_file(const QString& path) {
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
}

// TODO: index by samples instead of time
void AudioBuffer::sample_amplitude(int channel, double start_time, double duration, float& out_max, float& out_min) {
    Q_ASSERT(m_sample_rate > 0);
    Q_ASSERT(m_num_channels > 0);

    uint64_t sample_start = start_time * m_sample_rate;
    uint64_t num_samples = duration * (double) m_sample_rate;

    if (num_samples <= 0)
        num_samples = 1;

    float max = -2, min = 2;
    for (uint64_t i = sample_start; i < sample_start + num_samples; i++) {
        uint64_t index = i * m_num_channels + channel;
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

bool AudioBuffer::delete_region(uint64_t start, uint64_t end) {
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

void AudioBuffer::insert_silence(uint64_t where, uint64_t num_frames) {
    uint64_t num = num_frames * m_num_channels;
    for (uint64_t i = 0; i < num; i++) {
        m_samples.push_back(0);
    }
    on_length_changed();
}

bool AudioBuffer::copy_region(uint64_t start, uint64_t end, AudioBuffer& to) const {
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

bool AudioBuffer::cut_region(uint64_t start, uint64_t end, AudioBuffer& to) {
    Q_ASSERT(start < end);
    start = limit_bounds(start);
    end = limit_bounds(end);
    if (start == end)
        return false;

    copy_region(start, end, to);
    delete_region(start, end);
    return true;
}

bool AudioBuffer::paste_from(uint64_t where, const AudioBuffer& from) {
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

uint64_t AudioBuffer::limit_bounds(uint64_t frame_pos) const {
    return std::max(std::min(frame_pos, m_num_frames), (uint64_t) 0);
}
