#pragma once
#include "FlyPath.h"
#include <vector>
#include <utility>
#include <cmath>

namespace fly
{
enum Vehicle { vDot, vCar, vJet, vRocket, vTrain, vUfo, vBee, vNote, vehicleCount };
inline const char* vehicleNames[] = { "Dot", "Car", "Jet", "Rocket", "Train", "UFO", "Bee", "Music note" };

struct PresetData
{
    const char* name;
    std::vector<Vec3> pts;
    bool closed;
    int vehicle;
    std::vector<std::pair<const char*, float>> params; // values in real units (choice = index, bool = 0/1)
};

inline std::vector<Vec3> ellipse (float cx, float cy, float rx, float ry, float z, int n, float zWobble = 0.0f)
{
    std::vector<Vec3> v;
    for (int i = 0; i < n; ++i)
    {
        const float a = 1.5707963f - 6.2831853f * (float) i / (float) n; // start in front, go clockwise (to the right)
        v.push_back ({ cx + rx * std::cos (a), cy + ry * std::sin (a), z + zWobble * (float) (i % 2) });
    }
    return v;
}

inline std::vector<Vec3> figureEight()
{
    std::vector<Vec3> v;
    for (int i = 0; i < 8; ++i)
    {
        const float t = 6.2831853f * (float) i / 8.0f + 0.3926991f;
        const float s = std::sin (t), c = std::cos (t), d = 1.0f + s * s;
        v.push_back ({ 0.85f * c / d, 0.45f + 0.85f * s * c / d, 0.0f });
    }
    return v;
}

// mode: 0 Loop 1 Ping-Pong 2 One-Shot 3 Manual | trigger: 0 MIDI 1 Audio 2 Button | syncLen index: see FlyPath syncNames
inline const std::vector<PresetData>& factoryPresets()
{
    static const std::vector<PresetData> presets
    {
        { "Init - Basic pass", { { 0.9f, 0.35f, 0 }, { 0, 0.18f, 0 }, { -0.9f, 0.35f, 0 } }, false, vCar, {} },

        // --- realistic
        { "Car pass", { { 1, 0.25f, 0 }, { 0, 0.1f, 0 }, { -1, 0.25f, 0 } }, false, vCar,
          { { "sync", 0 }, { "lengthSec", 4.0f }, { "distance", 40 }, { "ground", 35 }, { "reverb", -24 } } },
        { "Highway, fast pass", { { 1, 0.15f, 0 }, { 0, 0.06f, 0 }, { -1, 0.15f, 0 } }, false, vCar,
          { { "sync", 0 }, { "lengthSec", 3.2f }, { "distance", 60 }, { "ground", 40 }, { "reverb", -26 } } },
        { "Train, long pass", { { 1, 0.12f, 0 }, { 0, 0.08f, 0 }, { -1, 0.12f, 0 } }, false, vTrain,
          { { "sync", 0 }, { "lengthSec", 12.0f }, { "distance", 80 }, { "ground", 30 }, { "width", 40 }, { "reverb", -18 }, { "revTime", 2.6f } } },
        { "Jet flyover", { { 0, 1, 0.45f }, { 0, 0, 0.6f }, { 0, -1, 0.45f } }, false, vJet,
          { { "sync", 0 }, { "lengthSec", 4.5f }, { "distance", 150 }, { "air", 120 }, { "ground", 50 }, { "reverb", -16 }, { "revTime", 3.2f } } },
        { "Rocket flyby", { { 1, 0.5f, 0.05f }, { 0, 0.08f, 0.02f }, { -1, -0.4f, 0.2f } }, false, vRocket,
          { { "sync", 0 }, { "lengthSec", 3.0f }, { "distance", 120 }, { "pitch", 130 }, { "ground", 25 }, { "reverb", -18 } } },
        { "Superfast flyby", { { 1, 0.1f, 0 }, { -1, 0.1f, 0 } }, false, vJet,
          { { "sync", 0 }, { "lengthSec", 1.9f }, { "distance", 60 }, { "pitch", 150 }, { "ground", 30 } } },
        { "Straight in", { { 0, 1, 0 }, { 0, 0.04f, 0 } }, false, vDot,
          { { "sync", 0 }, { "lengthSec", 8.0f }, { "distance", 60 } } },
        { "Straight away", { { 0, 0.04f, 0 }, { 0, 1, 0 } }, false, vDot,
          { { "sync", 0 }, { "lengthSec", 8.0f }, { "distance", 60 } } },
        { "U-turn in front", { { -0.5f, 1, 0 }, { -0.18f, 0.22f, 0 }, { 0.18f, 0.22f, 0 }, { 0.5f, 1, 0 } }, false, vCar,
          { { "sync", 0 }, { "lengthSec", 5.0f }, { "distance", 50 }, { "pitch", 140 }, { "ground", 30 } } },
        { "Short in, long away", { { -1, 0.6f, 0 }, { 0, 0.1f, 0 }, { 1, 0.9f, 0 } }, false, vCar,
          { { "sync", 0 }, { "lengthSec", 10.0f }, { "center", 25 }, { "distance", 50 } } },

        // --- music
        { "Music - Orbit around you", ellipse (0, 0, 0.5f, 0.5f, 0, 8), true, vNote,
          { { "syncLen", 4 }, { "distance", 10 }, { "reverb", -26 } } },
        { "Music - Figure 8", figureEight(), true, vNote,
          { { "syncLen", 5 }, { "distance", 14 }, { "pitch", 80 }, { "reverb", -24 } } },
        { "Music - Drop swoosh (MIDI)", { { -0.8f, -0.8f, 0.3f }, { 0, 0.1f, 0.05f }, { 0.9f, 0.9f, 0.1f } }, false, vNote,
          { { "mode", 2 }, { "syncLen", 4 }, { "center", 70 }, { "pitch", 130 }, { "distance", 35 }, { "ground", 0 }, { "reverb", -18 } } },
        { "Music - Riser fly-in", { { 0.3f, 1, 0.4f }, { 0.1f, 0.4f, 0.1f }, { 0, 0.05f, 0 } }, false, vRocket,
          { { "mode", 2 }, { "syncLen", 7 }, { "pitch", 200 }, { "distance", 80 }, { "ground", 0 }, { "reverb", -20 } } },
        { "Music - Orbiting arp", ellipse (0, 0.1f, 0.5f, 0.35f, 0, 8), true, vNote,
          { { "syncLen", 4 }, { "distance", 12 }, { "pitch", 60 }, { "gain", 50 }, { "width", 30 }, { "reverb", -24 } } },
        { "Music - Fly-over", { { 0, 1, 0.1f }, { 0, 0, 0.5f }, { 0, -1, 0.1f } }, false, vJet,
          { { "syncLen", 7 }, { "distance", 30 }, { "pitch", 70 }, { "ground", 10 } } },
        { "Music - Side to side", { { 1, 0.3f, 0 }, { -1, 0.3f, 0 } }, false, vNote,
          { { "mode", 1 }, { "syncLen", 4 }, { "distance", 12 }, { "pitch", 50 }, { "gain", 40 }, { "ground", 0 } } },

        // --- creative
        { "Spinner (Leslie fast)", { { -0.01f, 0.12f, 0 }, { 0.01f, 0.12f, 0 } }, false, vDot,
          { { "mode", 3 }, { "position", 50 }, { "distance", 10 }, { "spin", 100 }, { "spinRate", 6.7f }, { "ground", 0 }, { "reverb", -24 } } },
        { "Spinner (Leslie slow)", { { -0.01f, 0.12f, 0 }, { 0.01f, 0.12f, 0 } }, false, vDot,
          { { "mode", 3 }, { "position", 50 }, { "distance", 10 }, { "spin", 100 }, { "spinRate", 0.8f }, { "ground", 0 }, { "reverb", -24 } } },
        { "Pannerizer", ellipse (0, 0, 0.4f, 0.4f, 0, 8), true, vDot,
          { { "syncLen", 3 }, { "distance", 8 }, { "pitch", 0 }, { "gain", 0 }, { "air", 0 }, { "ground", 0 }, { "pan", 200 }, { "reverb", -60 } } },
        { "Jerky tape", { { 0, 0.7f, 0 }, { 0, 0.25f, 0 } }, false, vDot,
          { { "mode", 1 }, { "syncLen", 2 }, { "center", 20 }, { "distance", 20 }, { "pitch", 300 }, { "gain", 20 }, { "ground", 0 }, { "reverb", -60 } } },
        { "Physics anomaly - Reverse Doppler", { { 1, 0.25f, 0 }, { 0, 0.1f, 0 }, { -1, 0.25f, 0 } }, false, vUfo,
          { { "sync", 0 }, { "lengthSec", 4.0f }, { "distance", 40 }, { "pitch", -100 }, { "ground", 20 } } },
        { "Physics anomaly - Wormhole", ellipse (0, 0.4f, 0.3f, 0.3f, 0, 8), true, vUfo,
          { { "syncLen", 1 }, { "distance", 6 }, { "pitch", 300 }, { "gain", 40 }, { "ground", 0 }, { "reverb", -20 } } },
        { "Bee around your head", ellipse (0, 0, 0.6f, 0.45f, 0.15f, 8, 0.2f), true, vBee,
          { { "sync", 0 }, { "lengthSec", 2.5f }, { "distance", 2.5f }, { "pitch", 150 }, { "ground", 0 }, { "reverb", -30 } } },
        { "UFO hover", { { -0.4f, 0.5f, 0.3f }, { 0, 0.3f, 0.4f }, { 0.4f, 0.5f, 0.3f } }, false, vUfo,
          { { "mode", 1 }, { "syncLen", 9 }, { "distance", 25 }, { "pitch", 200 }, { "spin", 40 }, { "spinRate", 3.0f }, { "ground", 0 }, { "reverb", -14 }, { "revTime", 4.0f } } },
    };
    return presets;
}
} // namespace fly
