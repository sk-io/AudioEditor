#include "waveformvisual.h"

#include "app.h"
#include <math.h>
#include <algorithm>

WaveformVisual::WaveformVisual() {}

void WaveformVisual::render(int num_levels) {
    int64_t total_frames = the_app.buffer.get_num_frames();
    num_channels = the_app.buffer.get_num_channels();

    for (int i = 0; i < num_levels; i++) {
        int bucket_size = (int) pow(4, i + 2);
        Level level;
        level.bucket_size = bucket_size;

        for (int channel = 0; channel < num_channels; channel++) {
            // rounded up
            int64_t num_buckets = (total_frames + bucket_size - 1) / bucket_size;
            for (int b = 0; b < num_buckets; b++) {
                int64_t start_frame = b * bucket_size;
                int64_t end_frame = start_frame + bucket_size;

                float min, max;
                the_app.buffer.sample_amplitude(channel, start_frame, end_frame, max, min);
                level.buckets[channel].push_back(Bucket{
                    .min = min,
                    .max = max,
                });
            }
        }

        levels.push_back(std::move(level));
    }
}

// find coarsest zoom level for which:
//      bucket_size <= frames_per_pixel
int WaveformVisual::find_best_level(double frames_per_pixel) const {
    for (int i = levels.size() - 1; i >= 0; i--) {
        if (levels[i].bucket_size <= frames_per_pixel)
            return i;
    }

    return -1;
}

void WaveformVisual::sample(int64_t frame_start, int64_t frame_end, int level_i, int channel, float& min, float& max) const {
    const Level& level = levels[level_i];
    int64_t bucket_start = frame_start / level.bucket_size;
    int64_t bucket_end = frame_end / level.bucket_size;
    bucket_end = std::min(bucket_end, (int64_t) level.buckets[channel].size() - 1);

    min = level.buckets[channel][bucket_start].min;
    max = level.buckets[channel][bucket_start].max;
    for (int i = bucket_start + 1; i <= bucket_end; i++) {
        min = fmin(min, level.buckets[channel][i].min);
        max = fmax(max, level.buckets[channel][i].max);
    }
}
