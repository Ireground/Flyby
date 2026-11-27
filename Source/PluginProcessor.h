#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "FlyEngine.h"
#include "Presets.h"

namespace ParamIDs
{
    inline const juce::String mode = "mode", sync = "sync", syncLen = "syncLen", lengthSec = "lengthSec", center = "center",
        phase = "phase", reverse = "reverse", trigger = "trigger", threshold = "threshold", position = "position",
        distance = "distance", gain = "gain", pan = "pan", pitch = "pitch", air = "air", ground = "ground", width = "width",
        reverb = "reverb", revTime = "revTime", revBright = "revBright", spin = "spin", spinRate = "spinRate",
        keepInTime = "keepInTime", mix = "mix", output = "output";
}

class FlybyProcessor : public juce::AudioProcessor
{
public:
    FlybyProcessor();
    ~FlybyProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override {}
    bool supportsDoublePrecisionProcessing() const override { return false; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Flyby"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return (int) fly::factoryPresets().size(); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- used by the editor (message thread)
    juce::AudioProcessorValueTreeState apvts;
    fly::PathPoints getPoints() const { const juce::ScopedLock sl (pointsLock); return points; }
    void setPoints (const fly::PathPoints& pts);
    void fireTrigger() { engine.trigger(); }
    fly::EngineParams currentParams() const;
    float getParam (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    void setParam (const juce::String& id, float realValue);

    std::atomic<int> vehicle { fly::vCar };
    std::atomic<int> pathVersion { 0 };
    int editorWidth = 1180, editorHeight = 800;
    int currentPreset = 0;
    juce::String presetName { "Init - Basic pass" };

    struct SharedTelemetry
    {
        std::atomic<float> t { 0 }, u { 0 }, x { 0 }, y { 0 }, z { 0 }, semis { 0 }, kmh { 0 }, dist { 0 }, level { 0 }, elapsed { 0 }, passSec { 1 }, bpm { 120 };
        std::atomic<bool> running { true };
        std::atomic<int> counter { 0 };
    } tele;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void publishPath();

    fly::Engine engine;
    juce::CriticalSection pointsLock;
    fly::PathPoints points;

    juce::SpinLock tableLock;
    fly::PathTable pendingTable;
    std::atomic<bool> tableDirty { false };

    std::atomic<float>* rMode = nullptr; std::atomic<float>* rSync = nullptr; std::atomic<float>* rSyncLen = nullptr;
    std::atomic<float>* rLength = nullptr; std::atomic<float>* rCenter = nullptr; std::atomic<float>* rPhase = nullptr;
    std::atomic<float>* rReverse = nullptr; std::atomic<float>* rTrigger = nullptr; std::atomic<float>* rThreshold = nullptr;
    std::atomic<float>* rPosition = nullptr; std::atomic<float>* rDistance = nullptr; std::atomic<float>* rGain = nullptr;
    std::atomic<float>* rPan = nullptr; std::atomic<float>* rPitch = nullptr; std::atomic<float>* rAir = nullptr;
    std::atomic<float>* rGround = nullptr; std::atomic<float>* rWidth = nullptr; std::atomic<float>* rReverb = nullptr;
    std::atomic<float>* rRevTime = nullptr; std::atomic<float>* rRevBright = nullptr; std::atomic<float>* rSpin = nullptr;
    std::atomic<float>* rSpinRate = nullptr; std::atomic<float>* rKeep = nullptr; std::atomic<float>* rMix = nullptr;
    std::atomic<float>* rOutput = nullptr;
    std::array<int, 128> midiTrig {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlybyProcessor)
};
