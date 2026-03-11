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

    WaveformVisual();

    // TODO: do this?
    //void mark_dirty(int64_t start_frame, int64_t end_frame);
    void render(int num_levels);
    int find_best_level(double frames_per_pixel) const;
    void sample(int64_t frame_start, int64_t frame_end, int level_i, int channel, float& min, float& max) const;

    const int get_num_levels() const { return levels.size(); }
    bool is_ready() const {
        return !levels.empty();
    }

    const Level& get_level(int level) const {
        return levels[level];
    }

private:
    std::vector<Level> levels;
    int num_channels = -1;
};

#endif // WAVEFORMVISUAL_H
