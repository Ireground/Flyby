#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

namespace Pal
{
    const juce::Colour bgTop { 0xff22303f }, bgBottom { 0xff111a23 }, panel { 0xff0e161f }, panelEdge { 0xff2a3a4b },
        text { 0xffe9eef3 }, dim { 0xff8394a6 }, faint { 0xff3b4d60 }, blue { 0xff5ab4e8 }, gold { 0xfff2c14e },
        knobTop { 0xff3c4c5e }, knobBottom { 0xff17212b }, badge { 0xff0d151d }, red { 0xffe8645a }, green { 0xff6fd08c };
}

juce::Font uiFont (float h, bool bold = false);

//==============================================================================
class ProRLook : public juce::LookAndFeel_V4
{
public:
    ProRLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    float uiScale = 1.0f;
};

//==============================================================================
class Knob : public juce::Component, private juce::Slider::Listener
{
public:
    Knob (FlybyProcessor&, const juce::String& paramId, const juce::String& title,
          const juce::String& lo, const juce::String& hi, bool big = false);
    ~Knob() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseUp (const juce::MouseEvent&) override { repaint(); }

    juce::Slider slider;
private:
    void sliderValueChanged (juce::Slider*) override { repaint(); }
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    juce::String title, lo, hi;
    bool big;
};

//==============================================================================
class FieldView : public juce::Component
{
public:
    explicit FieldView (FlybyProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseMove (const juce::MouseEvent&) override;

    void tick();                 // called by the editor's timer
    void syncFromProcessor();
    const fly::PathTable& getTable() const { return table; }
    float uiScale = 1.0f;

private:
    juce::Point<float> toScreen (fly::Vec3 v, bool elevated = true) const;
    fly::Vec3 fromScreen (juce::Point<float> p) const;
    static constexpr float squash = 0.45f, heightFactor = 0.55f, skew = 0.32f;
    float heightPixels() const { return scale * heightFactor; }
    int hitPoint (juce::Point<float> p, bool& heightHandle) const;
    void commit();
    void showPointMenu (int index);
    void showFieldMenu();
    void insertPoint (fly::Vec3 v);

    FlybyProcessor& proc;
    fly::PathPoints pts;
    fly::PathTable table;
    int seenVersion = -1;

    juce::Rectangle<float> field;
    juce::Point<float> centre;
    float scale = 100.0f;

    int dragIndex = -1, hoverIndex = -1;
    bool dragHeight = false;
    float dragStartZ = 0, dragStartY = 0;

    juce::Array<fly::Vec3> trail;
    fly::Vec3 lastPos {};
    float heading = 0.0f;
};

//==============================================================================
class TimelineView : public juce::Component
{
public:
    explicit TimelineView (FlybyProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void tick (const fly::PathTable& tbl);
    float uiScale = 1.0f;
private:
    FlybyProcessor& proc;
    static constexpr int count = 240;
    std::array<float, count> semis {}, level {}, us {};
    float cursor = 0.0f, passSec = 1.0f;
    int mode = 0;
};

//==============================================================================
class FlyButton : public juce::Button
{
public:
    FlyButton() : juce::Button ("FLY") {}
    void paintButton (juce::Graphics&, bool over, bool down) override;
    float uiScale = 1.0f;
    juce::String label { "FLY!" };
    float flash = 0.0f;
};

//==============================================================================
class FlybyEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit FlybyEditor (FlybyProcessor&);
    ~FlybyEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void loadPreset (int index);
    void refreshPresetBox();

    FlybyProcessor& proc;
    ProRLook look;

    // bottom rows
    Knob distance, gain, pan, pitch, air, ground, width;
    Knob reverb, revTime, revBright, spin, spinRate, mix, output;
    // motion panel
    Knob lengthSec, center, phase, threshold, position;
    juce::ComboBox modeBox, syncLenBox, triggerBox, presetBox, vehicleBox;
    juce::ToggleButton syncToggle { "Tempo sync" }, reverseToggle { "Reverse direction" }, keepToggle { "On time at closest point" };
    juce::TextButton prevPreset { "<" }, nextPreset { ">" };
    FlyButton flyButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt, syncLenAtt, triggerAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAtt, reverseAtt, keepAtt;

    FieldView fieldView;
    TimelineView timeline;
    juce::TooltipWindow tooltips { this, 700 };

    juce::Rectangle<int> headerArea, panelArea, rowA, rowB, motionTitle, triggerTitle;
    float sc = 1.0f;
    int lastPreset = -1, lastCounter = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlybyEditor)
};
