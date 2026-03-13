#ifndef WAVEFORMVISUAL_H
#define WAVEFORMVISUAL_H

#include <vector>
#include <cstdint>

class WaveformVisual {
public:
    struct Bucket {
        float min, max;
    };

    struct Level {
        std::vector<Bucket> buckets[2];
        int bucket_size; // frames per bucket
    };

    WaveformVisual() {}

    void render(int64_t start_frame = 0, int64_t end_frame = -1);
    int find_best_level(double frames_per_pixel) const;
    void sample(int64_t frame_start, int64_t frame_end, int level_i, int channel, float& min, float& max) const;

    const int get_num_levels() const { return levels.size(); }

    const Level& get_level(int level) const {
        return levels[level];
    }

private:
    std::vector<Level> levels;
    const int num_levels = 2; // TODO: allow user to adjust?
    int num_channels = 0;
};

#endif // WAVEFORMVISUAL_H
