#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

static String pct (float v, int) { return String (roundToInt (v)) + " %"; }

// On/off switches are stored as plain 0..1 values (>= 0.5 = on) so any automation value restores exactly.
static std::unique_ptr<AudioParameterFloat> makeSwitch (const String& id, const String& name, bool on)
{
    return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f), on ? 1.0f : 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return v >= 0.5f ? String ("On") : String ("Off"); })
                                       .withValueFromStringFunction ([] (const String& t) { return t.trim().equalsIgnoreCase ("on") || t.getFloatValue() >= 0.5f ? 1.0f : 0.0f; }));
}

AudioProcessorValueTreeState::ParameterLayout FlybyProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    using F = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;
    auto pctAttr = A().withStringFromValueFunction (pct).withLabel ("%");

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::mode, 1 }, "Mode",
                                                         StringArray { "Loop", "Ping-Pong", "One-Shot", "Manual" }, 0));
    p.push_back (makeSwitch (ParamIDs::sync, "Tempo Sync", true));
    {
        StringArray names;
        for (auto* n : fly::syncNames) names.add (n);
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::syncLen, 1 }, "Pass Length (sync)", names, 5));
    }
    {
        NormalisableRange<float> r (0.1f, 60.0f, 0.0f);
        r.setSkewForCentre (5.0f);
        p.push_back (std::make_unique<F> (ParameterID { ParamIDs::lengthSec, 1 }, "Pass Length (sec)", r, 5.0f,
            A().withStringFromValueFunction ([] (float v, int) { return v < 10 ? String (v, 2) + " s" : String (v, 1) + " s"; }).withLabel ("s")));
    }
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::center, 1 }, "Center Time", NormalisableRange<float> (10, 90), 50, pctAttr));
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::phase, 1 }, "Loop Phase", NormalisableRange<float> (-50, 50), 0,
        A().withStringFromValueFunction ([] (float v, int) { return (v > 0.5f ? "+" : "") + String (roundToInt (v)) + " %"; })));
    p.push_back (makeSwitch (ParamIDs::reverse, "Reverse Direction", false));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::trigger, 1 }, "Trigger",
                                                         StringArray { "MIDI note", "Audio level", "Button only" }, 0));
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::threshold, 1 }, "Audio Threshold", NormalisableRange<float> (-60, 0), -24,
        A().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " dB"; })));
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::position, 1 }, "Position", NormalisableRange<float> (0, 100), 0, pctAttr));
    {
        NormalisableRange<float> r (2.0f, 300.0f, 0.0f);
        r.setSkewForCentre (30.0f);
        p.push_back (std::make_unique<F> (ParameterID { ParamIDs::distance, 1 }, "Distance", r, 30.0f,
            A().withStringFromValueFunction ([] (float v, int) { return v < 10 ? String (v, 1) + " m" : String (roundToInt (v)) + " m"; })));
    }
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::gain, 1 }, "Gain", NormalisableRange<float> (0, 200), 100, pctAttr));
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::pan, 1 }, "Pan", NormalisableRange<float> (0, 200), 100, pctAttr));
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::pitch, 1 }, "Pitch", NormalisableRange<float> (-100, 300), 100, pctAttr));
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::air, 1 }, "Air Damp", NormalisableRange<float> (0, 200), 100, pctAttr));
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::ground, 1 }, "Ground Reflection", NormalisableRange<float> (0, 100), 20, pctAttr));
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::width, 1 }, "Source Width", NormalisableRange<float> (0, 100), 0, pctAttr));
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::reverb, 1 }, "Reverb", NormalisableRange<float> (-60, 6), -22,
        A().withStringFromValueFunction ([] (float v, int) { return v <= -59.5f ? String ("Off") : String (v, 1) + " dB"; })));
    {
        NormalisableRange<float> r (0.2f, 10.0f, 0.0f);
        r.setSkewForCentre (2.0f);
        p.push_back (std::make_unique<F> (ParameterID { ParamIDs::revTime, 1 }, "Reverb Time", r, 1.6f,
            A().withStringFromValueFunction ([] (float v, int) { return String (v, 2) + " s"; })));
    }
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::revBright, 1 }, "Reverb Brightness", NormalisableRange<float> (0, 100), 50, pctAttr));
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::spin, 1 }, "Spin", NormalisableRange<float> (0, 100), 0, pctAttr));
    {
        NormalisableRange<float> r (0.1f, 12.0f, 0.0f);
        r.setSkewForCentre (2.0f);
        p.push_back (std::make_unique<F> (ParameterID { ParamIDs::spinRate, 1 }, "Spin Rate", r, 6.7f,
            A().withStringFromValueFunction ([] (float v, int) { return String (v, 2) + " Hz"; })));
    }
    p.push_back (makeSwitch (ParamIDs::keepInTime, "On Time At Closest Point", true));
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::mix, 1 }, "Mix", NormalisableRange<float> (0, 100), 100, pctAttr));
    p.push_back (std::make_unique<F> (ParameterID { ParamIDs::output, 1 }, "Output", NormalisableRange<float> (-24, 12), 0,
        A().withStringFromValueFunction ([] (float v, int) { return (v > 0.05f ? "+" : "") + String (v, 1) + " dB"; })));
    return { p.begin(), p.end() };
}

FlybyProcessor::FlybyProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "FLYBY", createLayout())
{
    auto r = [this] (const String& id) { return apvts.getRawParameterValue (id); };
    rMode = r (ParamIDs::mode); rSync = r (ParamIDs::sync); rSyncLen = r (ParamIDs::syncLen); rLength = r (ParamIDs::lengthSec);
    rCenter = r (ParamIDs::center); rPhase = r (ParamIDs::phase); rReverse = r (ParamIDs::reverse); rTrigger = r (ParamIDs::trigger);
    rThreshold = r (ParamIDs::threshold); rPosition = r (ParamIDs::position); rDistance = r (ParamIDs::distance); rGain = r (ParamIDs::gain);
    rPan = r (ParamIDs::pan); rPitch = r (ParamIDs::pitch); rAir = r (ParamIDs::air); rGround = r (ParamIDs::ground);
    rWidth = r (ParamIDs::width); rReverb = r (ParamIDs::reverb); rRevTime = r (ParamIDs::revTime); rRevBright = r (ParamIDs::revBright);
    rSpin = r (ParamIDs::spin); rSpinRate = r (ParamIDs::spinRate); rKeep = r (ParamIDs::keepInTime); rMix = r (ParamIDs::mix);
    rOutput = r (ParamIDs::output);

    const auto& pr = fly::factoryPresets()[0];
    points.count = (int) pr.pts.size();
    for (size_t i = 0; i < pr.pts.size(); ++i) points.p[i] = pr.pts[i];
    points.closed = pr.closed;
    publishPath();
}

bool FlybyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::stereo()) return false;
    const auto in = layouts.getMainInputChannelSet();
    return in == AudioChannelSet::stereo() || in == AudioChannelSet::mono();
}

void FlybyProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    {
        const SpinLock::ScopedLockType sl (tableLock);
        engine.setPath (pendingTable);
        tableDirty = false;
    }
    setLatencySamples (engine.latencySamples());
}

fly::EngineParams FlybyProcessor::currentParams() const
{
    fly::EngineParams e;
    e.mode = (int) rMode->load();
    e.sync = rSync->load() >= 0.5f;
    e.syncIndex = (int) rSyncLen->load();
    e.lengthSec = rLength->load();
    e.center = rCenter->load() * 0.01f;
    e.phase = rPhase->load() * 0.01f;
    e.reverse = rReverse->load() >= 0.5f;
    e.trigger = (int) rTrigger->load();
    e.thresholdDb = rThreshold->load();
    e.position = rPosition->load() * 0.01f;
    e.distance = rDistance->load();
    e.gain = rGain->load() * 0.01f;
    e.pan = rPan->load() * 0.01f;
    e.pitch = rPitch->load() * 0.01f;
    e.air = rAir->load() * 0.01f;
    e.ground = rGround->load() * 0.01f;
    e.width = rWidth->load() * 0.01f;
    e.reverbDb = rReverb->load();
    e.reverbTime = rRevTime->load();
    e.reverbBright = rRevBright->load() * 0.01f;
    e.spinDepth = rSpin->load() * 0.01f;
    e.spinRate = rSpinRate->load();
    e.keepInTime = rKeep->load() >= 0.5f;
    e.mix = rMix->load() * 0.01f;
    e.outputDb = rOutput->load();
    return e;
}

void FlybyProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
    if (n <= 0 || buffer.getNumChannels() < 2) return;
    if (numIn == 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);
    else if (numIn == 0) buffer.clear();

    if (tableDirty.load())
    {
        const SpinLock::ScopedTryLockType tl (tableLock);
        if (tl.isLocked()) { engine.setPath (pendingTable); tableDirty = false; }
    }

    int nTrig = 0;
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn() && nTrig < (int) midiTrig.size())
            midiTrig[(size_t) nTrig++] = jlimit (0, n - 1, meta.samplePosition);
    }
    std::sort (midiTrig.begin(), midiTrig.begin() + nTrig);

    fly::HostTime ht;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            ht.playing = pos->getIsPlaying();
            if (auto b = pos->getBpm()) ht.bpm = *b;
            if (auto q = pos->getPpqPosition()) { ht.ppq = *q; ht.hasPpq = true; }
            if (auto ts = pos->getTimeSignature()) { ht.num = jmax (1, ts->numerator); ht.den = jmax (1, ts->denominator); }
        }

    engine.process (buffer.getWritePointer (0), buffer.getWritePointer (1), n, currentParams(), ht, midiTrig.data(), nTrig);

    const auto& t = engine.telemetry;
    tele.t = t.t; tele.u = t.u; tele.x = t.x; tele.y = t.y; tele.z = t.z;
    tele.semis = t.pitchSemis; tele.kmh = t.speedKmh; tele.dist = t.distanceM; tele.level = t.levelDb;
    tele.elapsed = t.elapsed; tele.passSec = t.passSec; tele.running = t.running; tele.bpm = (float) ht.bpm;
    tele.counter.fetch_add (1);
}

void FlybyProcessor::publishPath()
{
    fly::PathPoints copy;
    {
        const ScopedLock sl (pointsLock);
        copy = points;
    }
    auto tbl = std::make_unique<fly::PathTable>();
    tbl->build (copy);
    {
        const SpinLock::ScopedLockType sl (tableLock);
        pendingTable = *tbl;
        tableDirty = true;
    }
    pathVersion.fetch_add (1);
}

void FlybyProcessor::setPoints (const fly::PathPoints& pts)
{
    {
        const ScopedLock sl (pointsLock);
        points = pts;
        points.count = jlimit (fly::minPoints, fly::maxPoints, points.count);
        for (auto& v : points.p) { v.x = jlimit (-1.0f, 1.0f, v.x); v.y = jlimit (-1.0f, 1.0f, v.y); v.z = jlimit (0.0f, 1.0f, v.z); }
    }
    publishPath();
}

void FlybyProcessor::setParam (const String& id, float realValue)
{
    if (auto* prm = apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (realValue));
        prm->endChangeGesture();
    }
}

void FlybyProcessor::setCurrentProgram (int index)
{
    const auto& all = fly::factoryPresets();
    if (! isPositiveAndBelow (index, (int) all.size())) return;
    const auto& pr = all[(size_t) index];
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<RangedAudioParameter*> (prm))
        {
            rp->beginChangeGesture();
            rp->setValueNotifyingHost (rp->getDefaultValue());
            rp->endChangeGesture();
        }
    for (const auto& [id, v] : pr.params) setParam (id, v);
    fly::PathPoints pts;
    pts.count = jlimit (fly::minPoints, fly::maxPoints, (int) pr.pts.size());
    for (int i = 0; i < pts.count; ++i) pts.p[(size_t) i] = pr.pts[(size_t) i];
    pts.closed = pr.closed;
    setPoints (pts);
    vehicle = pr.vehicle;
    currentPreset = index;
    presetName = pr.name;
}

const String FlybyProcessor::getProgramName (int index)
{
    const auto& all = fly::factoryPresets();
    return isPositiveAndBelow (index, (int) all.size()) ? String (all[(size_t) index].name) : String();
}

void FlybyProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("PATH"), nullptr);
    ValueTree path ("PATH");
    {
        const ScopedLock sl (pointsLock);
        path.setProperty ("closed", points.closed, nullptr);
        for (int i = 0; i < points.count; ++i)
        {
            ValueTree pt ("P");
            pt.setProperty ("x", points.p[(size_t) i].x, nullptr);
            pt.setProperty ("y", points.p[(size_t) i].y, nullptr);
            pt.setProperty ("z", points.p[(size_t) i].z, nullptr);
            path.appendChild (pt, nullptr);
        }
    }
    state.appendChild (path, nullptr);
    state.setProperty ("vehicle", vehicle.load(), nullptr);
    state.setProperty ("editorW", editorWidth, nullptr);
    state.setProperty ("editorH", editorHeight, nullptr);
    state.setProperty ("preset", currentPreset, nullptr);
    state.setProperty ("presetName", presetName, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void FlybyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    auto state = ValueTree::fromXml (*xml);
    if (! state.isValid()) return;

    auto path = state.getChildWithName ("PATH");
    if (path.isValid() && path.getNumChildren() >= fly::minPoints)
    {
        fly::PathPoints pts;
        pts.closed = (bool) path.getProperty ("closed", false);
        pts.count = jmin (fly::maxPoints, path.getNumChildren());
        for (int i = 0; i < pts.count; ++i)
        {
            auto c = path.getChild (i);
            pts.p[(size_t) i] = { (float) c.getProperty ("x", 0.0f), (float) c.getProperty ("y", 0.5f), (float) c.getProperty ("z", 0.0f) };
        }
        setPoints (pts);
    }
    vehicle = jlimit (0, fly::vehicleCount - 1, (int) state.getProperty ("vehicle", (int) fly::vCar));
    editorWidth = (int) state.getProperty ("editorW", editorWidth);
    editorHeight = (int) state.getProperty ("editorH", editorHeight);
    currentPreset = jlimit (0, getNumPrograms() - 1, (int) state.getProperty ("preset", 0));
    presetName = state.getProperty ("presetName", presetName).toString();
    state.removeChild (state.getChildWithName ("PATH"), nullptr);
    apvts.replaceState (state);
}

AudioProcessorEditor* FlybyProcessor::createEditor() { return new FlybyEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FlybyProcessor(); }
