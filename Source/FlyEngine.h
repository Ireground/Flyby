#pragma once
// Flyby audio engine. Real Doppler: the sound is written into a delay line and read back at the
// travel time from the moving source to each ear, so pitch, timing and ear differences come out of
// the physics instead of a pitch shifter.
#include "FlyPath.h"
#include <vector>
#include <cstdint>
#include <atomic>

namespace fly
{
struct EngineParams
{
    int mode = 0;            // 0 Loop, 1 Ping-Pong, 2 One-Shot, 3 Manual
    bool sync = true;
    int syncIndex = 5;       // index into syncBars()
    float lengthSec = 5.0f;
    float center = 0.5f;     // when the middle of the path is reached (0.1..0.9)
    float phase = 0.0f;      // loop offset (-0.5..0.5)
    bool reverse = false;
    int trigger = 0;         // 0 MIDI note, 1 Audio level, 2 Button only
    float thresholdDb = -24.0f;
    float position = 0.0f;   // Manual mode
    float distance = 40.0f;  // field radius in metres
    float gain = 1.0f, pan = 1.0f, pitch = 1.0f, air = 1.0f; // 1 = realistic
    float ground = 0.2f, width = 0.0f;
    float reverbDb = -20.0f, reverbTime = 1.8f, reverbBright = 0.5f;
    float spinDepth = 0.0f, spinRate = 6.7f;
    bool keepInTime = true;
    float mix = 1.0f, outputDb = 0.0f;
};

struct HostTime
{
    bool playing = false, hasPpq = false;
    double ppq = 0.0, bpm = 120.0;
    int num = 4, den = 4;
};

struct Telemetry
{
    float t = 0, u = 0, x = 0, y = 0, z = 0;   // time through the cycle, current path position (field units)
    float pitchSemis = 0, speedKmh = 0, distanceM = 0, levelDb = 0;
    float elapsed = 0, passSec = 1;
    bool running = true;
};

inline float passSeconds (const EngineParams& p, double bpm, int num, int den)
{
    if (! p.sync) return std::max (0.05f, p.lengthSec);
    const int idx = std::max (0, std::min ((int) syncBars().size() - 1, p.syncIndex));
    const double beatsPerBar = (double) num * 4.0 / (double) std::max (1, den);
    const double beats = (double) syncBars()[(size_t) idx] * beatsPerBar;
    return (float) std::max (0.02, beats * 60.0 / std::max (20.0, bpm));
}

//==============================================================================
class FracDelay
{
public:
    void prepare (int minSize)
    {
        int s = 1;
        while (s < minSize) s <<= 1;
        buf.assign ((size_t) s, 0.0f);
        mask = s - 1;
        w = 0;
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
    int size() const { return mask + 1; }
    inline void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }

    // d >= 2 samples behind the newest written sample (4-point Hermite interpolation)
    inline float read (float d) const
    {
        const int di = (int) d;
        const float f = 1.0f - (d - (float) di);
        const int i = w - 2 - di;
        const float xm1 = buf[(size_t) ((i - 1) & mask)], x0 = buf[(size_t) (i & mask)],
                    x1 = buf[(size_t) ((i + 1) & mask)], x2 = buf[(size_t) ((i + 2) & mask)];
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }
    inline float readInt (int d) const { return buf[(size_t) ((w - 1 - d) & mask)]; }

private:
    std::vector<float> buf { 0.0f };
    int mask = 0, w = 0;
};

//==============================================================================
// Small 8-line feedback delay network for the distance reverb.
class Fdn
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        for (int k = 0; k < 8; ++k)
        {
            len[(size_t) k] = (int) (baseMs[k] * 0.001 * fs);
            lines[(size_t) k].assign ((size_t) len[(size_t) k] + 1, 0.0f);
        }
        for (int k = 0; k < 4; ++k)
        {
            apLen[(size_t) k] = (int) (apMs[k] * 0.001 * fs);
            ap[(size_t) k].assign ((size_t) apLen[(size_t) k] + 1, 0.0f);
        }
        reset();
    }
    void reset()
    {
        for (auto& l : lines) std::fill (l.begin(), l.end(), 0.0f);
        for (auto& l : ap) std::fill (l.begin(), l.end(), 0.0f);
        idx.fill (0); apIdx.fill (0); lp.fill (0.0f);
    }
    void set (float rt, float bright)
    {
        rt = std::max (0.1f, rt);
        for (int k = 0; k < 8; ++k)
            g[(size_t) k] = std::pow (10.0f, -3.0f * (float) len[(size_t) k] / (float) fs / rt);
        const float fc = 1200.0f + bright * bright * 15000.0f;
        damp = 1.0f - std::exp (-2.0f * 3.14159265f * std::min (fc, (float) fs * 0.45f) / (float) fs);
    }
    inline void process (float x, float& outL, float& outR)
    {
        for (int k = 0; k < 4; ++k)  // input diffusion
        {
            auto& b = ap[(size_t) k];
            int& i = apIdx[(size_t) k];
            const float d = b[(size_t) i];
            const float v = x + 0.62f * d;
            b[(size_t) i] = v;
            x = d - 0.62f * v;
            if (++i >= apLen[(size_t) k]) i = 0;
        }
        std::array<float, 8> o;
        for (int k = 0; k < 8; ++k) o[(size_t) k] = lines[(size_t) k][(size_t) idx[(size_t) k]];
        // 8-point Hadamard
        for (int s = 1; s < 8; s <<= 1)
            for (int i = 0; i < 8; i += 2 * s)
                for (int j = i; j < i + s; ++j)
                {
                    const float a = o[(size_t) j], b = o[(size_t) (j + s)];
                    o[(size_t) j] = a + b; o[(size_t) (j + s)] = a - b;
                }
        outL = outR = 0.0f;
        for (int k = 0; k < 8; ++k)
        {
            const float back = lines[(size_t) k][(size_t) idx[(size_t) k]];
            (k & 1 ? outR : outL) += back;
            lp[(size_t) k] += damp * (o[(size_t) k] * 0.35355339f * g[(size_t) k] - lp[(size_t) k]);
            lines[(size_t) k][(size_t) idx[(size_t) k]] = lp[(size_t) k] + x * 0.5f;
            if (++idx[(size_t) k] >= len[(size_t) k]) idx[(size_t) k] = 0;
        }
        outL *= 0.5f; outR *= 0.5f;
    }

private:
    static constexpr double baseMs[8] { 33.1, 39.7, 44.3, 50.9, 56.3, 63.7, 70.1, 77.9 };
    static constexpr double apMs[4] { 4.7, 7.3, 11.3, 13.9 };
    double fs = 48000.0;
    std::array<std::vector<float>, 8> lines;
    std::array<std::vector<float>, 4> ap;
    std::array<int, 8> len {}, idx {};
    std::array<int, 4> apLen {}, apIdx {};
    std::array<float, 8> g {}, lp {};
    float damp = 0.5f;
};

//==============================================================================
class Engine
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate > 1000.0 ? sampleRate : 48000.0;
        base = std::max (8, (int) std::lround (0.004 * fs));
        const int size = (int) (maxDelaySec * fs) + base + 16;
        lineL.prepare (size); lineR.prepare (size);
        dryL.prepare (base + 8); dryR.prepare (base + 8);
        maxD = (float) (lineL.size() - 8);
        fdn.prepare (fs);
        reset();
    }

    int latencySamples() const { return base; }

    void reset()
    {
        lineL.reset(); lineR.reset(); dryL.reset(); dryR.reset(); fdn.reset();
        for (auto& v : voices) v = Voice();
        t = 0.0; running = false; env = 0.0f; armed = true; holdoff = 0;
        spinTheta = 0.0f; prevTheta = 0.0f; prevDc = -1.0f; haveMotion = false; lastPosM = {}; havePos = false;
        widthV = -1.0f; mixV = -1.0f; outV = -1.0f;
        telemetry = {};
    }

    void setPath (const PathTable& tbl) { table = tbl; distCacheF = -1.0f; }
    const PathTable& getPath() const { return table; }

    void trigger() { pendingTrigger = true; }

    // L/R: in-place stereo buffers. trig: sample offsets of note-on triggers (ascending).
    void process (float* L, float* R, int n, const EngineParams& prm, const HostTime& host, const int* trig, int nTrig)
    {
        if (n <= 0) return;
        p = prm;
        updateCaches (host);

        // tempo-locked phase while the host plays
        const double bpm = host.bpm > 0 ? host.bpm : 120.0;
        passSec = passSeconds (p, bpm, host.num, host.den);
        const bool pingpong = p.mode == 1;
        const double period = (double) passSec * (pingpong ? 2.0 : 1.0);
        dt = 1.0 / (period * fs);
        if (p.sync && host.playing && host.hasPpq && (p.mode == 0 || p.mode == 1))
        {
            const double beatsPerBar = (double) host.num * 4.0 / (double) std::max (1, host.den);
            const double periodBeats = (double) syncBars()[(size_t) std::max (0, std::min (13, p.syncIndex))] * beatsPerBar * (pingpong ? 2.0 : 1.0);
            const double ph = host.ppq / std::max (1.0e-6, periodBeats);
            t = ph - std::floor (ph);
            syncLocked = true;
        }
        else syncLocked = false;

        if (pendingTrigger.exchange (false)) doTrigger();

        const float thrLin = std::pow (10.0f, p.thresholdDb / 20.0f);
        const float envRel = std::exp (-1.0f / (0.04f * (float) fs));
        const float revLin = p.reverbDb <= -59.5f ? 0.0f : std::pow (10.0f, p.reverbDb / 20.0f);
        if (revLin > 0.0f) fdn.set (p.reverbTime, p.reverbBright);
        else if (revWasOn) fdn.reset();
        revWasOn = revLin > 0.0f;

        const float tgtWidth = clampf (p.width, 0, 1), tgtMix = clampf (p.mix, 0, 1);
        const float tgtOut = std::pow (10.0f, clampf (p.outputDb, -60, 24) / 20.0f);
        if (widthV < 0) { widthV = tgtWidth; mixV = tgtMix; outV = tgtOut; }
        const float sm = 1.0f - std::exp (-1.0f / (0.01f * (float) fs));

        int pos = 0, ti = 0;
        while (pos < n)
        {
            while (ti < nTrig && trig[ti] <= pos) { if (p.trigger == 0) doTrigger(); ++ti; }
            if (p.trigger == 1 && holdoff <= 0 && armed && env > thrLin) { doTrigger(); armed = false; holdoff = (int) (0.08 * fs); }

            int len = std::min (control, n - pos);
            if (ti < nTrig && trig[ti] > pos) len = std::min (len, trig[ti] - pos);

            advanceMotion (len);
            planChunk (len, revLin);

            for (int i = pos; i < pos + len; ++i)
            {
                const float l = L[i], r = R[i];
                widthV += sm * (tgtWidth - widthV);
                mixV += sm * (tgtMix - mixV);
                outV += sm * (tgtOut - outV);
                const float M = 0.5f * (l + r), S = 0.5f * (l - r);
                lineL.push (M + widthV * S);
                lineR.push (M - widthV * S);
                dryL.push (l); dryR.push (r);

                const float a = std::abs (M);
                env = a > env ? a : env * envRel;

                float oL = 0, oR = 0, send = 0;
                for (auto& v : voices)
                {
                    if (! v.active) continue;
                    float y[4];
                    for (int k = 0; k < 4; ++k)
                    {
                        auto& tp = v.tap[k];
                        tp.d = std::min (maxD, std::max (2.0f, tp.d + tp.dStep));
                        tp.g += tp.gStep;
                        const float x = (k & 1) ? lineR.read (tp.d) : lineL.read (tp.d);
                        tp.z1 += tp.a * (x - tp.z1);
                        tp.z2 += tp.a * (tp.z1 - tp.z2);
                        y[k] = tp.z2;
                    }
                    v.send += v.sendStep;
                    const float f = v.fade;
                    oL += (y[0] * v.tap[0].g + y[2] * v.tap[2].g) * f;
                    oR += (y[1] * v.tap[1].g + y[3] * v.tap[3].g) * f;
                    send += (y[0] + y[1]) * 0.5f * v.send * f;
                    v.fade += v.fadeStep;
                    if (v.fadeStep > 0 && v.fade >= 1.0f) { v.fade = 1.0f; v.fadeStep = 0; }
                    if (v.fadeStep < 0 && v.fade <= 0.0f) v.active = false;
                }
                if (revLin > 0.0f)
                {
                    float rl, rr;
                    fdn.process (send, rl, rr);
                    oL += rl; oR += rr;
                }
                const float dl = dryL.readInt (base), dr = dryR.readInt (base);
                float outL = (dl + (oL - dl) * mixV) * outV;
                float outR = (dr + (oR - dr) * mixV) * outV;
                if (! std::isfinite (outL) || ! std::isfinite (outR)) { outL = outR = 0; panicReset(); }
                L[i] = clampf (outL, -8.0f, 8.0f);
                R[i] = clampf (outR, -8.0f, 8.0f);
            }
            if (holdoff > 0) holdoff -= len;
            if (! armed && env < thrLin * 0.5f) armed = true;
            pos += len;
        }

        telemetry.t = p.mode == 3 ? clampf (p.position, 0, 1) : (float) t;
        telemetry.u = curU;
        telemetry.x = curPos.x; telemetry.y = curPos.y; telemetry.z = curPos.z;
        telemetry.passSec = passSec;
        telemetry.running = p.mode == 2 ? running : true;
        telemetry.elapsed = (float) (shownT * passSec);
    }

    Telemetry telemetry;

private:
    struct Tap { float d = 0, dStep = 0, g = 0, gStep = 0, a = 1, z1 = 0, z2 = 0; };
    struct Voice { bool active = false; float fade = 0, fadeStep = 0; Tap tap[4]; float send = 0, sendStep = 0; };
    struct Targets { float d[4], g[4], a[4], send; float dCentre; };

    void panicReset()
    {
        lineL.reset(); lineR.reset(); fdn.reset();
        for (auto& v : voices) v = Voice();
    }

    void doTrigger()
    {
        if (p.mode == 3) return;
        if (syncLocked && (p.mode == 0 || p.mode == 1)) return; // locked to the song position
        t = 0.0;
        running = true;
    }

    void updateCaches (const HostTime&)
    {
        if (std::abs (warp.center - p.center) > 1.0e-4f) warp.build (p.center);
        if (std::abs (distCacheF - p.distance) > 1.0e-4f)
        {
            distCacheF = p.distance;
            float mn = 1.0e9f, mx = 0.0f;
            for (const auto& v : table.pos)
            {
                const float d = distanceFromHead (toMetres (v, p.distance));
                mn = std::min (mn, d); mx = std::max (mx, d);
            }
            dMin = mn; dMax = std::max (mn, mx);
        }
    }

    double modeT (double tt) const
    {
        if (p.mode == 0) { tt += p.phase; return tt - std::floor (tt); }
        if (p.mode == 1) { tt += p.phase; tt -= std::floor (tt); return tt < 0.5 ? 2.0 * tt : 2.0 - 2.0 * tt; }
        return std::min (1.0, std::max (0.0, tt));
    }

    void advanceMotion (int len)
    {
        if (p.mode == 2)
        {
            if (running) { t += dt * len; if (t >= 1.0) { t = 1.0; running = false; } }
        }
        else if (p.mode != 3)
        {
            t += dt * len;
            if (t >= 1.0) t -= std::floor (t);
        }
        float u;
        if (p.mode == 3) { u = clampf (p.position, 0, 1); shownT = u; }
        else { const double mt = modeT (t); shownT = mt; u = warp.map ((float) mt); }
        if (p.reverse) u = 1.0f - u;
        curU = u;
        const Vec3 before = curPos;
        curPos = table.at (u);
        // Where the source was one chunk ago (for its speed). Taken from the path itself, so a loop
        // restart (teleport) never looks like supersonic motion: use the side that has no jump.
        if (p.mode == 3 || ! haveMotion) velPrevPos = before;
        else
        {
            const Vec3 back = posAtT (t - dt * len), fwd = posAtT (t + dt * len);
            const float jb = (curPos - back).length(), jf = (fwd - curPos).length();
            velPrevPos = (jb > 4.0f * jf + 1.0e-4f) ? curPos - (fwd - curPos) : back;
        }
        if (! haveMotion) velPrevPos = curPos;
        haveMotion = true;
        prevTheta = spinTheta;
        spinTheta += 6.2831853f * p.spinRate * (float) len / (float) fs;
        if (spinTheta > 6.2831853f) { spinTheta -= 6.2831853f; prevTheta -= 6.2831853f; }
    }

    Vec3 posAtT (double tt) const
    {
        if (p.mode == 2) tt = std::min (1.0, std::max (0.0, tt));
        else tt -= std::floor (tt);
        float u = warp.map ((float) modeT (tt));
        if (p.reverse) u = 1.0f - u;
        return table.at (u);
    }

    static float coefFor (float invFc, double fs)
    {
        const float fc = clampf (1.0f / (invFc + 1.0f / 30000.0f), 120.0f, (float) fs * 0.45f);
        return 1.0f - std::exp (-6.2831853f * fc / (float) fs);
    }

    Targets computeTargets (float revLin, int len)
    {
        Targets tg {};
        const float F = std::max (0.5f, p.distance);
        Vec3 m = toMetres (curPos, F), mp = toMetres (velPrevPos, F);
        const float rs = 0.25f * clampf (p.spinDepth, 0, 1);
        m.x += rs * std::cos (spinTheta);
        m.y += rs * std::sin (spinTheta);
        mp.x += rs * std::cos (prevTheta);
        mp.y += rs * std::sin (prevTheta);

        const float panAmt = clampf (p.pan, 0, 2);
        const float eh = 0.09f * panAmt;
        const Vec3 earL { -eh, 0, earHeight }, earR { eh, 0, earHeight }, head { 0, 0, earHeight };
        const Vec3 img { m.x, m.y, -m.z };
        const float dL = (m - earL).length(), dR = (m - earR).length(), dC = (m - head).length();
        const float rL = (img - earL).length(), rR = (img - earR).length(), rC = (img - head).length();
        const Vec3 imgP { mp.x, mp.y, -mp.z };
        const float dPrev[5] = { (mp - earL).length(), (mp - earR).length(), (imgP - earL).length(), (imgP - earR).length(), (mp - head).length() };

        const float P = clampf (p.pitch, -1.0f, 3.0f);
        const float negOff = P < 0 ? -P * (dMax - dMin) : 0.0f;
        const float phys = p.keepInTime ? 0.0f : dMin;
        // Sound heard now left the source earlier: delay = d / (c + radial speed). This gives the exact
        // moving-source Doppler shift c / (c + v) instead of the moving-listener approximation.
        const float chunkSec = (float) len / (float) fs;
        const float dNow[5] = { dL, dR, rL, rR, dC };
        float delays[5];
        for (int i = 0; i < 5; ++i)
        {
            float vr = (dNow[i] - dPrev[i]) / chunkSec;
            if (std::abs (vr) > 2.0f * speedOfSound) vr = 0.0f;   // a jump (e.g. dragging in Manual mode), not motion
            const float denom = std::max (0.25f * speedOfSound, speedOfSound + P * vr);
            delays[i] = (float) base + (float) fs * (P * (dNow[i] - dMin) / denom + (negOff + phys) / speedOfSound);
        }
        for (int i = 0; i < 4; ++i) tg.d[i] = clampf (delays[i], 2.0f, maxD);
        tg.dCentre = delays[4];

        const float dref = std::max (dMin, 0.5f);
        const float gainAmt = clampf (p.gain, 0, 2);
        const float gDist = std::min (4.0f, std::pow (dref / std::max (dC, 0.5f), gainAmt));
        const float gRef = clampf (p.ground, 0, 1) * 0.75f * std::min (4.0f, std::pow (dref / std::max (rC, 0.5f), gainAmt));

        const float inv = 1.0f / std::max (dC, 1.0e-3f);
        const float side = m.x * inv, front = m.y * inv;
        const float pp = clampf (side * panAmt * 0.6f, -1.0f, 1.0f);
        const float th = (pp + 1.0f) * 0.78539816f;
        const float gpL = 1.41421356f * std::cos (th), gpR = 1.41421356f * std::sin (th);

        // spinning horn (Leslie-style): louder and brighter when it faces you
        const float depth = clampf (p.spinDepth, 0, 1);
        float facing = 0.0f;
        {
            const float hx = std::cos (spinTheta), hy = std::sin (spinTheta);
            const float lx = -m.x, ly = -m.y, ll = std::sqrt (lx * lx + ly * ly);
            facing = ll > 1.0e-4f ? (hx * lx + hy * ly) / ll : 0.0f;
        }
        const float gSpin = 1.0f + 0.45f * depth * facing;
        const float invSpin = depth * (1.0f - facing) * 0.5f / 2500.0f;

        auto invAir = [&] (float d)
        {
            const float A = 0.1f * d * clampf (p.air, 0, 2);   // dB lost at 10 kHz
            return std::sqrt (std::max (0.0f, std::pow (10.0f, A / 20.0f) - 1.0f)) / 10000.0f;
        };
        const float shadowL = pp > 0 ? pp / 4000.0f : 0.0f, shadowR = pp < 0 ? -pp / 4000.0f : 0.0f;
        const float back = std::max (0.0f, -front) * std::min (panAmt, 1.0f) / 9000.0f;
        const float common = back + invSpin;

        tg.a[0] = coefFor (invAir (dL) + shadowL + common, fs);
        tg.a[1] = coefFor (invAir (dR) + shadowR + common, fs);
        tg.a[2] = coefFor (invAir (rL) + shadowL + common + 1.0f / 6000.0f, fs);
        tg.a[3] = coefFor (invAir (rR) + shadowR + common + 1.0f / 6000.0f, fs);
        tg.g[0] = gDist * gpL * gSpin; tg.g[1] = gDist * gpR * gSpin;
        tg.g[2] = gRef * gpL * gSpin;  tg.g[3] = gRef * gpR * gSpin;
        tg.send = revLin * gSpin;
        return tg;
    }

    void planChunk (int len, float revLin)
    {
        const Targets tg = computeTargets (revLin, len);
        auto& v = voices[0];
        const float inv = 1.0f / (float) len;

        bool jump = ! v.active;
        if (v.active)
            for (int k = 0; k < 4; ++k)
                if (std::abs (tg.d[k] - v.tap[k].d) > 6.0f * (float) len + 48.0f) jump = true;

        if (jump)
        {
            const bool first = ! v.active;
            if (! first)
            {
                // old position keeps playing and fades out; use a free slot, or replace the quietest one
                size_t slot = 1;
                for (size_t i = 1; i < voices.size(); ++i)
                {
                    if (! voices[i].active) { slot = i; break; }
                    if (voices[i].fade < voices[slot].fade) slot = i;
                }
                auto& o = voices[slot];
                o = v;
                o.fadeStep = -o.fade / (0.03f * (float) fs);
                if (o.fadeStep > -1.0e-6f) o.fadeStep = -1.0f / (0.03f * (float) fs);
                for (auto& tp : o.tap) { tp.dStep = 0; tp.gStep = 0; }
                o.sendStep = 0;
            }
            v = Voice();
            v.active = true;
            v.fade = first ? 1.0f : 0.0f;
            v.fadeStep = first ? 0.0f : 1.0f / (0.03f * (float) fs);
            for (int k = 0; k < 4; ++k) { v.tap[k].d = tg.d[k]; v.tap[k].g = tg.g[k]; v.tap[k].a = tg.a[k]; }
            v.send = tg.send;
            prevDc = tg.dCentre;
        }
        else
        {
            for (int k = 0; k < 4; ++k)
            {
                auto& tp = v.tap[k];
                tp.dStep = clampf ((tg.d[k] - tp.d) * inv, -3.0f, 0.75f);  // pitch limited to +/- 2 octaves
                tp.gStep = (tg.g[k] - tp.g) * inv;
                tp.a = tg.a[k];
            }
            v.sendStep = (tg.send - v.send) * inv;
        }

        // telemetry
        const float rate = prevDc < 0 ? 0.0f : (tg.dCentre - prevDc) * inv;
        prevDc = tg.dCentre;
        const float ratio = clampf (1.0f - rate, 0.25f, 4.0f);
        const float semis = 12.0f * std::log2 (ratio);
        telemetry.pitchSemis += 0.15f * (semis - telemetry.pitchSemis);
        const Vec3 m = toMetres (curPos, std::max (0.5f, p.distance));
        if (havePos)
        {
            const float sp = (m - lastPosM).length() / ((float) len / (float) fs) * 3.6f;
            telemetry.speedKmh += 0.05f * (sp - telemetry.speedKmh);
        }
        lastPosM = m; havePos = true;
        telemetry.distanceM = distanceFromHead (m);
        telemetry.levelDb = 20.0f * std::log10 (std::max (1.0e-5f, 0.5f * (tg.g[0] + tg.g[1])));
    }

    static constexpr double maxDelaySec = 6.0;
    static constexpr int control = 16;

    double fs = 48000.0, t = 0.0, dt = 0.0, shownT = 0.0;
    int base = 192, holdoff = 0;
    float maxD = 1000.0f, passSec = 1.0f;
    bool running = false, armed = true, syncLocked = false, revWasOn = false, havePos = false;
    std::atomic<bool> pendingTrigger { false };
    EngineParams p;
    PathTable table;
    Warp warp;
    float distCacheF = -1.0f, dMin = 1.0f, dMax = 1.0f;
    FracDelay lineL, lineR, dryL, dryR;
    Fdn fdn;
    std::array<Voice, 3> voices;
    float env = 0.0f, spinTheta = 0.0f, prevDc = -1.0f, curU = 0.0f;
    float prevTheta = 0.0f;
    Vec3 velPrevPos {};
    bool haveMotion = false;
    Vec3 curPos {}, lastPosM {};
    float widthV = -1, mixV = -1, outV = -1;
};

//==============================================================================
// Predicted pitch (semitones) and level (dB) across one pass, for the editor's timeline strip.
inline void predictPass (const PathTable& tbl, const EngineParams& p, float passSec, int count, float* semis, float* levelDb, float* uOut)
{
    Warp w; w.build (p.center);
    const float F = std::max (0.5f, p.distance);
    float mn = 1.0e9f;
    for (const auto& v : tbl.pos) mn = std::min (mn, distanceFromHead (toMetres (v, F)));
    const float dref = std::max (mn, 0.5f);
    const float P = clampf (p.pitch, -1, 3);
    auto uAt = [&] (float tt)
    {
        float mt;
        if (p.mode == 3) mt = tt;
        else if (p.mode == 1) { float x = tt + p.phase; x -= std::floor (x); mt = x < 0.5f ? 2 * x : 2 - 2 * x; }
        else if (p.mode == 0) { float x = tt + p.phase; mt = x - std::floor (x); }
        else mt = tt;
        float u = p.mode == 3 ? mt : w.map (mt);
        return p.reverse ? 1.0f - u : u;
    };
    const float period = passSec * (p.mode == 1 ? 2.0f : 1.0f);
    const float h = 1.0f / (float) (count * 4);
    for (int i = 0; i < count; ++i)
    {
        const float tt = (float) i / (float) (count - 1);
        const float u = uAt (tt);
        const float d = distanceFromHead (toMetres (tbl.at (u), F));
        const float t0 = std::max (0.0f, tt - h), t1 = std::min (1.0f, tt + h);
        const float d0 = distanceFromHead (toMetres (tbl.at (uAt (t0)), F)), d1 = distanceFromHead (toMetres (tbl.at (uAt (t1)), F));
        float rate = 0.0f;
        if (t1 > t0 && p.mode != 3) rate = P * (d1 - d0) / speedOfSound / ((t1 - t0) * period);
        // ignore loop-wrap teleports
        if (std::abs (rate) > 1.5f) rate = 0.0f;
        const float ratio = clampf (1.0f - rate, 0.25f, 4.0f);
        semis[i] = 12.0f * std::log2 (ratio);
        levelDb[i] = 20.0f * std::log10 (std::max (1.0e-5f, std::min (4.0f, std::pow (dref / std::max (d, 0.5f), clampf (p.gain, 0, 2)))));
        if (uOut) uOut[i] = u;
    }
}
} // namespace fly
