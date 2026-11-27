#pragma once
// Flyby - path geometry and timing shared by the audio engine and the editor.
// Field coordinates: x = left(-1) .. right(+1), y = behind(-1) .. in front(+1), z = height 0 (ground) .. 1.
// The listener sits at the centre (0, 0). Metres = field units * Distance (except height offsets below).
#include <array>
#include <cmath>
#include <algorithm>
#include <vector>

namespace fly
{
struct Vec3
{
    float x = 0, y = 0, z = 0;
    Vec3 operator+ (Vec3 o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vec3 operator- (Vec3 o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vec3 operator* (float s) const { return { x * s, y * s, z * s }; }
    float length() const { return std::sqrt (x * x + y * y + z * z); }
};

constexpr int maxPoints = 12;
constexpr int minPoints = 2;
constexpr int tableSize = 1024;           // arc-length table resolution
constexpr float sourceBaseHeight = 0.6f;  // metres above ground for a source at z = 0
constexpr float earHeight = 1.7f;         // listener ear height in metres
constexpr float speedOfSound = 343.0f;

struct PathPoints
{
    int count = 3;
    std::array<Vec3, maxPoints> p {};
    bool closed = false;
};

inline float clampf (float v, float lo, float hi) { return std::min (hi, std::max (lo, v)); }

inline Vec3 catmull (Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3, float t)
{
    const float t2 = t * t, t3 = t2 * t;
    auto f = [&] (float a, float b, float c, float d)
    { return 0.5f * ((2 * b) + (-a + c) * t + (2 * a - 5 * b + 4 * c - d) * t2 + (-a + 3 * b - 3 * c + d) * t3); };
    return { f (p0.x, p1.x, p2.x, p3.x), f (p0.y, p1.y, p2.y, p3.y), f (p0.z, p1.z, p2.z, p3.z) };
}

// Path resampled to equal arc-length steps, so the source moves at constant speed along it.
struct PathTable
{
    std::array<Vec3, tableSize + 1> pos {};
    bool closed = false;
    float length = 1.0f; // in field units

    Vec3 at (float u) const
    {
        if (closed) u = u - std::floor (u);
        else u = clampf (u, 0.0f, 1.0f);
        const float f = u * (float) tableSize;
        int i = (int) f;
        if (i >= tableSize) return pos[(size_t) tableSize];
        const float a = f - (float) i;
        const auto& p0 = pos[(size_t) i];
        const auto& p1 = pos[(size_t) i + 1];
        return p0 + (p1 - p0) * a;
    }

    void build (const PathPoints& pts)
    {
        closed = pts.closed && pts.count >= 3;
        const int n = std::max (minPoints, std::min (maxPoints, pts.count));
        auto P = [&] (int i) -> Vec3
        {
            if (closed) { i %= n; if (i < 0) i += n; return pts.p[(size_t) i]; }
            if (i < 0) return pts.p[0] * 2.0f - pts.p[1];                         // reflected end tangents
            if (i >= n) return pts.p[(size_t) n - 1] * 2.0f - pts.p[(size_t) n - 2];
            return pts.p[(size_t) i];
        };
        const int segs = closed ? n : n - 1;
        constexpr int sub = 96;
        std::vector<Vec3> dense;
        dense.reserve ((size_t) (segs * sub + 1));
        for (int s = 0; s < segs; ++s)
            for (int k = 0; k < sub; ++k)
            {
                Vec3 v = catmull (P (s - 1), P (s), P (s + 1), P (s + 2), (float) k / (float) sub);
                v.z = std::max (0.0f, v.z);
                dense.push_back (v);
            }
        dense.push_back (closed ? pts.p[0] : pts.p[(size_t) n - 1]);

        std::vector<float> cum (dense.size(), 0.0f);
        for (size_t i = 1; i < dense.size(); ++i) cum[i] = cum[i - 1] + (dense[i] - dense[i - 1]).length();
        length = std::max (1.0e-4f, cum.back());

        size_t j = 0;
        for (int i = 0; i <= tableSize; ++i)
        {
            const float target = length * (float) i / (float) tableSize;
            while (j + 1 < dense.size() - 1 && cum[j + 1] < target) ++j;
            const float seg = std::max (1.0e-9f, cum[j + 1] - cum[j]);
            const float a = clampf ((target - cum[j]) / seg, 0.0f, 1.0f);
            pos[(size_t) i] = dense[j] + (dense[j + 1] - dense[j]) * a;
        }
        if (cum.back() < 1.0e-4f) for (auto& v : pos) v = dense[0];
    }
};

// Converts field units to metres around the listener's head (listener ears at height earHeight).
inline Vec3 toMetres (Vec3 v, float distance) { return { v.x * distance, v.y * distance, sourceBaseHeight + v.z * distance }; }

inline float distanceFromHead (Vec3 m) { return std::sqrt (m.x * m.x + m.y * m.y + (m.z - earHeight) * (m.z - earHeight)); }

// Timing warp: "Center" (0.1..0.9) is when the source reaches the middle of the path.
// Speed changes smoothly from the approach speed to the leaving speed.
struct Warp
{
    static constexpr int size = 256;
    std::array<float, size + 1> table {};
    float center = -1.0f;

    void build (float c)
    {
        c = clampf (c, 0.05f, 0.95f);
        center = c;
        const float v1 = 0.5f / c, v2 = 0.5f / (1.0f - c);
        const float w = 0.12f;
        constexpr int steps = 2048;
        std::array<float, steps + 1> acc {};
        for (int i = 1; i <= steps; ++i)
        {
            const float t = ((float) i - 0.5f) / (float) steps;
            float s = clampf ((t - (c - w)) / (2 * w), 0.0f, 1.0f);
            s = s * s * (3 - 2 * s);
            acc[(size_t) i] = acc[(size_t) i - 1] + (v1 + (v2 - v1) * s) / (float) steps;
        }
        for (int i = 0; i <= size; ++i) table[(size_t) i] = acc[(size_t) (i * steps / size)] / acc[steps];
    }

    float map (float t) const
    {
        t = clampf (t, 0.0f, 1.0f);
        const float f = t * (float) size;
        const int i = std::min (size - 1, (int) f);
        const float a = f - (float) i;
        return table[(size_t) i] + (table[(size_t) i + 1] - table[(size_t) i]) * a;
    }
};

// Pass length choices for tempo sync, in bars.
inline const std::array<float, 14>& syncBars()
{
    static const std::array<float, 14> b { 1.0f / 16, 1.0f / 8, 1.0f / 4, 1.0f / 2, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32 };
    return b;
}
inline const char* syncNames[] = { "1/16 bar", "1/8 bar", "1/4 bar", "1/2 bar", "1 bar", "2 bars", "3 bars", "4 bars",
                                   "6 bars", "8 bars", "12 bars", "16 bars", "24 bars", "32 bars" };
} // namespace fly
