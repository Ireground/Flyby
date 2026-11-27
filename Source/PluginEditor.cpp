#include "PluginEditor.h"

using namespace juce;

Font uiFont (float h, bool bold)
{
    auto o = FontOptions (h);
    return Font (bold ? o.withStyle ("Bold") : o);
}

//==============================================================================
ProRLook::ProRLook()
{
    setColour (PopupMenu::backgroundColourId, Pal::panel);
    setColour (PopupMenu::textColourId, Pal::text);
    setColour (PopupMenu::headerTextColourId, Pal::gold);
    setColour (PopupMenu::highlightedBackgroundColourId, Pal::blue.withAlpha (0.35f));
    setColour (PopupMenu::highlightedTextColourId, Colours::white);
    setColour (ComboBox::textColourId, Pal::text);
    setColour (ComboBox::backgroundColourId, Pal::panel);
    setColour (ComboBox::outlineColourId, Pal::panelEdge);
    setColour (ComboBox::arrowColourId, Pal::dim);
    setColour (TextButton::textColourOffId, Pal::text);
    setColour (ToggleButton::textColourId, Pal::dim);
    setColour (TooltipWindow::backgroundColourId, Pal::panel);
    setColour (TooltipWindow::textColourId, Pal::text);
    setColour (TooltipWindow::outlineColourId, Pal::panelEdge);
}

void ProRLook::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end, Slider& s)
{
    const bool big = (bool) s.getProperties()["big"];
    const bool bipolar = (bool) s.getProperties()["bipolar"];
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float r = jmin (b.getWidth(), b.getHeight()) * 0.5f * 0.78f;
    const auto c = b.getCentre();
    const float angle = start + pos * (end - start);

    g.setColour (Pal::faint);
    const int ticks = big ? 21 : 11;
    for (int i = 0; i < ticks; ++i)
    {
        const float a = start + (float) i / (float) (ticks - 1) * (end - start);
        const auto p = c.getPointOnCircumference (r * 1.2f, a);
        g.fillEllipse (p.x - 1.2f * uiScale, p.y - 1.2f * uiScale, 2.4f * uiScale, 2.4f * uiScale);
    }

    {
        const float zeroPos = bipolar ? (float) s.getProperties()["zero"] : 0.0f;
        const float a0 = start + zeroPos * (end - start);
        Path arc;
        arc.addCentredArc (c.x, c.y, r * 1.08f, r * 1.08f, 0.0f, jmin (a0, angle), jmax (a0, angle), true);
        g.setColour (Pal::blue.withAlpha (s.isMouseOverOrDragging() ? 1.0f : 0.75f));
        g.strokePath (arc, PathStrokeType (2.0f * uiScale, PathStrokeType::curved, PathStrokeType::rounded));
    }

    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillEllipse (c.x - r, c.y - r + 2.0f * uiScale, r * 2, r * 2);
    ColourGradient grad (Pal::knobTop, c.x, c.y - r, Pal::knobBottom, c.x, c.y + r, false);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.drawEllipse (c.x - r + 0.5f, c.y - r + 0.5f, r * 2 - 1, r * 2 - 1, 1.0f);
    if (big)
    {
        const float ri = r * 0.82f;
        g.setColour (Colour (0xff131c26));
        g.fillEllipse (c.x - ri, c.y - ri, ri * 2, ri * 2);
        g.setColour (Pal::text);
        g.setFont (uiFont (r * 0.32f, true));
        g.drawText (s.getTextFromValue (s.getValue()), Rectangle<float> (c.x - ri, c.y - r * 0.2f, ri * 2, r * 0.4f), Justification::centred);
    }

    const auto p1 = c.getPointOnCircumference (big ? r * 0.86f : r * 0.3f, angle);
    const auto p2 = c.getPointOnCircumference (r * 0.95f, angle);
    g.setColour (Colours::white);
    g.drawLine ({ p1, p2 }, (big ? 3.0f : 2.4f) * uiScale);
}

void ProRLook::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? Pal::blue.withAlpha (0.25f) : (over ? Colour (0xff1a2633) : Pal::panel));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (over ? Pal::blue.withAlpha (0.7f) : Pal::panelEdge);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
}

void ProRLook::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    g.setColour (Pal::text);
    g.setFont (uiFont (13.0f * uiScale, true));
    g.drawText (b.getButtonText(), b.getLocalBounds(), Justification::centred);
}

void ProRLook::drawToggleButton (Graphics& g, ToggleButton& b, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat();
    const float h = 14.0f * uiScale, w = 26.0f * uiScale;
    auto sw = Rectangle<float> (r.getX(), r.getCentreY() - h * 0.5f, w, h);
    const bool on = b.getToggleState();
    g.setColour (on ? Pal::blue.withAlpha (0.85f) : Pal::faint);
    g.fillRoundedRectangle (sw, h * 0.5f);
    const float d = h - 4.0f;
    g.setColour (Colours::white);
    g.fillEllipse (on ? sw.getRight() - d - 2.0f : sw.getX() + 2.0f, sw.getY() + 2.0f, d, d);
    g.setColour (over ? Pal::text : Pal::dim);
    g.setFont (uiFont (11.5f * uiScale, true));
    g.drawText (b.getButtonText().toUpperCase(), r.withTrimmedLeft (w + 8.0f * uiScale), Justification::centredLeft);
}

void ProRLook::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.isMouseOver (true) ? Colour (0xff1a2633) : Pal::panel);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (box.isMouseOver (true) ? Pal::blue.withAlpha (0.7f) : Pal::panelEdge);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
    Path arrow;
    const float ax = (float) w - 16.0f * uiScale, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4 * uiScale, ay - 2 * uiScale, ax + 4 * uiScale, ay - 2 * uiScale, ax, ay + 3 * uiScale);
    g.setColour (Pal::dim);
    g.fillPath (arrow);
}

Font ProRLook::getComboBoxFont (ComboBox&) { return uiFont (13.0f * uiScale, true); }
Font ProRLook::getPopupMenuFont() { return uiFont (14.0f * uiScale); }
void ProRLook::positionComboBoxText (ComboBox& box, Label& l)
{
    l.setBounds ((int) (12 * uiScale), 0, box.getWidth() - (int) (30 * uiScale), box.getHeight());
    l.setFont (getComboBoxFont (box));
}

//==============================================================================
Knob::Knob (FlybyProcessor& p, const String& paramId, const String& t, const String& l, const String& h, bool isBig)
    : title (t), lo (l), hi (h), big (isBig)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    slider.getProperties().set ("big", big);
    slider.setMouseDragSensitivity (big ? 260 : 200);
    addAndMakeVisible (slider);
    attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, paramId, slider);
    if (auto* param = p.apvts.getParameter (paramId))
    {
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        const auto range = param->getNormalisableRange();
        if (range.start < 0.0f && range.end > 0.0f)
        {
            slider.getProperties().set ("bipolar", true);
            slider.getProperties().set ("zero", range.convertTo0to1 (0.0f));
        }
    }
    slider.addListener (this);
    slider.addMouseListener (this, false);
}

Knob::~Knob() { slider.removeListener (this); }

void Knob::resized()
{
    auto b = getLocalBounds();
    const int labelH = jmax (14, getHeight() / 6);
    b.removeFromBottom (labelH);
    slider.setBounds (b);
}

void Knob::paint (Graphics& g)
{
    const int labelH = jmax (14, getHeight() / 6);
    auto lab = getLocalBounds().removeFromBottom (labelH).toFloat();
    const bool active = slider.isMouseOverOrDragging();
    const float fh = labelH * 0.72f;

    g.setFont (uiFont (fh, true));
    if (active && ! big)
    {
        g.setColour (Pal::blue);
        g.drawText (slider.getTextFromValue (slider.getValue()), lab, Justification::centred);
    }
    else
    {
        g.setColour (Pal::text);
        g.drawText (title, lab, Justification::centred);
    }

    auto sb = slider.getBounds().toFloat();
    const float r = jmin (sb.getWidth(), sb.getHeight()) * 0.5f * 0.78f;
    const auto c = sb.getCentre();
    g.setColour (Pal::dim);
    g.setFont (uiFont (fh * 0.72f, true));
    const float yy = c.y + r * 1.0f;
    g.drawText (lo, Rectangle<float> (c.x - r * 2.2f, yy, r * 1.6f, fh), Justification::centredRight);
    g.drawText (hi, Rectangle<float> (c.x + r * 0.6f, yy, r * 1.6f, fh), Justification::centredLeft);
}

//==============================================================================
// Vehicles, drawn top-down and pointing "up" before rotation.
static void drawVehicle (Graphics& g, int type, Point<float> c, float s, float heading, float time)
{
    const auto xf = AffineTransform::rotation (heading).translated (c);
    auto fillShape = [&] (const Path& p, Colour col) { g.setColour (col); g.fillPath (p, xf); };

    switch (type)
    {
        case fly::vCar:
        {
            Path body; body.addRoundedRectangle (-0.26f * s, -0.5f * s, 0.52f * s, s, 0.12f * s);
            Path wheels;
            for (float wy : { -0.3f, 0.28f })
                for (float wx : { -0.31f, 0.21f })
                    wheels.addRoundedRectangle (wx * s, wy * s, 0.1f * s, 0.2f * s, 0.03f * s);
            fillShape (wheels, Colour (0xff0a0f14));
            fillShape (body, Pal::blue);
            Path glass; glass.addRoundedRectangle (-0.2f * s, -0.28f * s, 0.4f * s, 0.18f * s, 0.05f * s);
            glass.addRoundedRectangle (-0.2f * s, 0.2f * s, 0.4f * s, 0.12f * s, 0.04f * s);
            fillShape (glass, Colour (0xff10202e));
            Path lights; lights.addEllipse (-0.22f * s, -0.52f * s, 0.12f * s, 0.06f * s); lights.addEllipse (0.1f * s, -0.52f * s, 0.12f * s, 0.06f * s);
            fillShape (lights, Pal::gold);
            break;
        }
        case fly::vJet:
        {
            Path p;
            p.startNewSubPath (0, -0.55f * s);
            p.lineTo (0.07f * s, -0.3f * s); p.lineTo (0.07f * s, -0.05f * s); p.lineTo (0.55f * s, 0.12f * s);
            p.lineTo (0.55f * s, 0.2f * s); p.lineTo (0.07f * s, 0.12f * s); p.lineTo (0.06f * s, 0.36f * s);
            p.lineTo (0.22f * s, 0.46f * s); p.lineTo (0.22f * s, 0.52f * s); p.lineTo (0, 0.47f * s);
            p.lineTo (-0.22f * s, 0.52f * s); p.lineTo (-0.22f * s, 0.46f * s); p.lineTo (-0.06f * s, 0.36f * s);
            p.lineTo (-0.07f * s, 0.12f * s); p.lineTo (-0.55f * s, 0.2f * s); p.lineTo (-0.55f * s, 0.12f * s);
            p.lineTo (-0.07f * s, -0.05f * s); p.lineTo (-0.07f * s, -0.3f * s); p.closeSubPath();
            fillShape (p, Colour (0xffd8e2ec));
            Path cockpit; cockpit.addEllipse (-0.035f * s, -0.42f * s, 0.07f * s, 0.14f * s);
            fillShape (cockpit, Pal::blue);
            break;
        }
        case fly::vRocket:
        {
            Path flame;
            const float fl = 0.25f + 0.1f * std::sin (time * 40.0f);
            flame.addTriangle (-0.1f * s, 0.36f * s, 0.1f * s, 0.36f * s, 0, (0.36f + fl) * s);
            fillShape (flame, Colour (0xffff8a3d));
            Path fins;
            fins.addTriangle (-0.12f * s, 0.1f * s, -0.26f * s, 0.38f * s, -0.12f * s, 0.32f * s);
            fins.addTriangle (0.12f * s, 0.1f * s, 0.26f * s, 0.38f * s, 0.12f * s, 0.32f * s);
            fillShape (fins, Pal::red);
            Path body; body.addRoundedRectangle (-0.12f * s, -0.3f * s, 0.24f * s, 0.66f * s, 0.05f * s);
            body.addTriangle (-0.12f * s, -0.29f * s, 0.12f * s, -0.29f * s, 0, -0.55f * s);
            fillShape (body, Colour (0xffe6edf3));
            Path win; win.addEllipse (-0.06f * s, -0.18f * s, 0.12f * s, 0.12f * s);
            fillShape (win, Pal::blue);
            break;
        }
        case fly::vTrain:
        {
            Path body;
            for (int i = 0; i < 3; ++i) body.addRoundedRectangle (-0.16f * s, (-0.75f + 0.5f * (float) i) * s, 0.32f * s, 0.46f * s, 0.05f * s);
            fillShape (body, Pal::blue);
            Path nose; nose.addRoundedRectangle (-0.12f * s, -0.73f * s, 0.24f * s, 0.1f * s, 0.04f * s);
            fillShape (nose, Pal::gold);
            break;
        }
        case fly::vUfo:
        {
            const auto t2 = AffineTransform::translation (c);
            Path saucer; saucer.addEllipse (-0.5f * s, -0.24f * s, s, 0.48f * s);
            g.setColour (Colour (0xffb9c6d3)); g.fillPath (saucer, t2);
            Path dome; dome.addEllipse (-0.2f * s, -0.2f * s, 0.4f * s, 0.4f * s);
            g.setColour (Pal::blue.withAlpha (0.9f)); g.fillPath (dome, t2);
            for (int i = 0; i < 8; ++i)
            {
                const float a = (float) i / 8.0f * MathConstants<float>::twoPi + time * 3.0f;
                const float lx = std::cos (a) * 0.38f * s, ly = std::sin (a) * 0.17f * s;
                g.setColour (Pal::gold.withAlpha (0.5f + 0.5f * std::sin (time * 8.0f + (float) i)));
                g.fillEllipse (c.x + lx - 0.035f * s, c.y + ly - 0.035f * s, 0.07f * s, 0.07f * s);
            }
            break;
        }
        case fly::vBee:
        {
            const float flap = 0.75f + 0.25f * std::sin (time * 60.0f);
            Path wings; wings.addEllipse (-0.45f * s * flap, -0.2f * s, 0.4f * s * flap, 0.26f * s); wings.addEllipse (0.05f * s, -0.2f * s, 0.4f * s * flap, 0.26f * s);
            fillShape (wings, Colours::white.withAlpha (0.55f));
            Path body; body.addEllipse (-0.17f * s, -0.4f * s, 0.34f * s, 0.8f * s);
            fillShape (body, Pal::gold);
            Path stripes;
            for (float sy : { -0.12f, 0.05f, 0.2f }) stripes.addRectangle (-0.17f * s, sy * s, 0.34f * s, 0.07f * s);
            g.saveState(); g.reduceClipRegion (body, xf);
            fillShape (stripes, Colour (0xff1a1a1a));
            g.restoreState();
            break;
        }
        case fly::vNote:
        {
            const auto t2 = AffineTransform::translation (c);
            Path n; n.addEllipse (-0.32f * s, 0.08f * s, 0.34f * s, 0.26f * s);
            n.addRectangle (-0.02f * s, -0.5f * s, 0.06f * s, 0.7f * s);
            Path flag; flag.startNewSubPath (0.04f * s, -0.5f * s);
            flag.quadraticTo (0.36f * s, -0.36f * s, 0.24f * s, -0.08f * s);
            flag.quadraticTo (0.24f * s, -0.28f * s, 0.04f * s, -0.32f * s); flag.closeSubPath();
            n.addPath (flag);
            g.setColour (Pal::gold); g.fillPath (n, t2);
            break;
        }
        default:
        {
            g.setColour (Pal::gold.withAlpha (0.25f));
            g.fillEllipse (c.x - 0.5f * s, c.y - 0.5f * s, s, s);
            g.setColour (Pal::gold);
            g.fillEllipse (c.x - 0.25f * s, c.y - 0.25f * s, 0.5f * s, 0.5f * s);
            break;
        }
    }
}

//==============================================================================
FieldView::FieldView (FlybyProcessor& p) : proc (p) { syncFromProcessor(); }

void FieldView::resized()
{
    field = getLocalBounds().toFloat();
    auto inner = field.reduced (14.0f * uiScale);
    inner.removeFromTop (6.0f * uiScale);
    inner.removeFromBottom (22.0f * uiScale);   // hint line
    // floor seen at an angle: depth is squashed, height goes straight up
    scale = jmin (inner.getWidth() * 0.5f / (1.0f + skew), inner.getHeight() / (2.0f * squash + heightFactor)) * 0.97f;
    centre = { inner.getCentreX(), inner.getBottom() - squash * scale };
}

Point<float> FieldView::toScreen (fly::Vec3 v, bool elevated) const
{
    return { centre.x + (v.x + skew * v.y) * scale, centre.y - v.y * scale * squash - (elevated ? v.z * heightPixels() : 0.0f) };
}

fly::Vec3 FieldView::fromScreen (Point<float> p) const
{
    const float y = (centre.y - p.y) / (scale * squash);
    return { jlimit (-1.0f, 1.0f, (p.x - centre.x) / scale - skew * y), jlimit (-1.0f, 1.0f, y), 0.0f };
}

void FieldView::syncFromProcessor()
{
    const int v = proc.pathVersion.load();
    if (v == seenVersion) return;
    seenVersion = v;
    pts = proc.getPoints();
    table.build (pts);
    repaint();
}

void FieldView::commit()
{
    proc.setPoints (pts);
    seenVersion = proc.pathVersion.load();
    table.build (pts);
    repaint();
}

int FieldView::hitPoint (Point<float> p, bool& heightHandle) const
{
    const float r = 9.0f * uiScale;
    for (int i = pts.count - 1; i >= 0; --i)
    {
        const auto v = pts.p[(size_t) i];
        if (v.z > 0.005f && toScreen (v, true).getDistanceFrom (p) < r) { heightHandle = true; return i; }
        if (toScreen (v, false).getDistanceFrom (p) < r) { heightHandle = false; return i; }
    }
    return -1;
}

void FieldView::insertPoint (fly::Vec3 v)
{
    if (pts.count >= fly::maxPoints) return;
    const int n = pts.count;
    const int segs = pts.closed ? n : n - 1;
    int best = n - 1; float bestD = 1.0e9f, bestT = 1.0f;
    for (int i = 0; i < segs; ++i)
    {
        const auto a = pts.p[(size_t) i], b = pts.p[(size_t) ((i + 1) % n)];
        const float dx = b.x - a.x, dy = b.y - a.y, l2 = dx * dx + dy * dy;
        float t = l2 > 0 ? ((v.x - a.x) * dx + (v.y - a.y) * dy) / l2 : 0.0f;
        const float tc = jlimit (0.0f, 1.0f, t);
        const float px = a.x + dx * tc - v.x, py = a.y + dy * tc - v.y;
        const float d = px * px + py * py;
        if (d < bestD) { bestD = d; best = i; bestT = t; }
    }
    int at = best + 1;
    if (! pts.closed && best == 0 && bestT < 0.0f) at = 0;
    if (! pts.closed && best == segs - 1 && bestT > 1.0f) at = n;
    // new point takes the height of its neighbours
    const auto a = pts.p[(size_t) jlimit (0, n - 1, at - 1)], b = pts.p[(size_t) jlimit (0, n - 1, at % n)];
    v.z = 0.5f * (a.z + b.z);
    for (int i = n; i > at; --i) pts.p[(size_t) i] = pts.p[(size_t) i - 1];
    pts.p[(size_t) at] = v;
    pts.count = n + 1;
    commit();
}

void FieldView::mouseDown (const MouseEvent& e)
{
    bool hh = false;
    const int i = hitPoint (e.position, hh);
    if (e.mods.isPopupMenu())
    {
        if (i >= 0) showPointMenu (i); else showFieldMenu();
        return;
    }
    dragIndex = i;
    dragHeight = hh || e.mods.isShiftDown();
    if (i >= 0) { dragStartZ = pts.p[(size_t) i].z; dragStartY = e.position.y; }
}

void FieldView::mouseDrag (const MouseEvent& e)
{
    if (dragIndex < 0) return;
    auto& v = pts.p[(size_t) dragIndex];
    if (dragHeight)
        v.z = jlimit (0.0f, 1.0f, dragStartZ + (dragStartY - e.position.y) / heightPixels());
    else
    {
        const auto g = fromScreen (e.position);
        v.x = g.x; v.y = g.y;
    }
    commit();
}

void FieldView::mouseUp (const MouseEvent&) { dragIndex = -1; }

void FieldView::mouseMove (const MouseEvent& e)
{
    bool hh = false;
    const int i = hitPoint (e.position, hh);
    if (i != hoverIndex) { hoverIndex = i; repaint(); }
    setMouseCursor (i >= 0 ? (hh ? MouseCursor::UpDownResizeCursor : MouseCursor::DraggingHandCursor) : MouseCursor::NormalCursor);
}

void FieldView::mouseDoubleClick (const MouseEvent& e)
{
    bool hh = false;
    const int i = hitPoint (e.position, hh);
    if (i >= 0)
    {
        if (pts.count <= fly::minPoints) return;
        for (int k = i; k < pts.count - 1; ++k) pts.p[(size_t) k] = pts.p[(size_t) k + 1];
        --pts.count;
        if (pts.count < 3) pts.closed = false;
        dragIndex = -1;
        commit();
    }
    else insertPoint (fromScreen (e.position));
}

void FieldView::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    bool hh = false;
    const int i = hitPoint (e.position, hh);
    if (i < 0) return;
    auto& v = pts.p[(size_t) i];
    v.z = jlimit (0.0f, 1.0f, v.z + w.deltaY * 0.15f);
    commit();
}

void FieldView::showPointMenu (int index)
{
    PopupMenu m;
    m.addSectionHeader ("POINT " + String (index + 1));
    m.addItem (1, "Delete point", pts.count > fly::minPoints);
    m.addItem (2, "Put on the ground", pts.p[(size_t) index].z > 0.0f);
    m.addItem (3, "Raise to half height");
    m.showMenuAsync (PopupMenu::Options().withMousePosition(), [safe = Component::SafePointer<FieldView> (this), index] (int r)
    {
        if (safe == nullptr || r == 0 || index >= safe->pts.count) return;
        auto& pp = safe->pts;
        if (r == 1)
        {
            for (int k = index; k < pp.count - 1; ++k) pp.p[(size_t) k] = pp.p[(size_t) k + 1];
            --pp.count;
            if (pp.count < 3) pp.closed = false;
        }
        if (r == 2) pp.p[(size_t) index].z = 0.0f;
        if (r == 3) pp.p[(size_t) index].z = 0.5f;
        safe->commit();
    });
}

void FieldView::showFieldMenu()
{
    PopupMenu m;
    m.addSectionHeader ("PATH");
    m.addItem (1, "Closed loop (start = end)", pts.count >= 3, pts.closed);
    m.addItem (2, "Reverse path order");
    m.addItem (3, "Mirror left / right");
    m.addItem (4, "Mirror front / back");
    m.addItem (5, "Put whole path on the ground");
    m.addSeparator();
    m.addItem (6, "Reset to basic pass");
    m.showMenuAsync (PopupMenu::Options().withMousePosition(), [safe = Component::SafePointer<FieldView> (this)] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& pp = safe->pts;
        const int n = pp.count;
        switch (r)
        {
            case 1: pp.closed = ! pp.closed; break;
            case 2: std::reverse (pp.p.begin(), pp.p.begin() + n); break;
            case 3: for (int i = 0; i < n; ++i) pp.p[(size_t) i].x = -pp.p[(size_t) i].x; break;
            case 4: for (int i = 0; i < n; ++i) pp.p[(size_t) i].y = -pp.p[(size_t) i].y; break;
            case 5: for (int i = 0; i < n; ++i) pp.p[(size_t) i].z = 0.0f; break;
            case 6: pp = {}; pp.count = 3; pp.p[0] = { 0.9f, 0.35f, 0 }; pp.p[1] = { 0, 0.18f, 0 }; pp.p[2] = { -0.9f, 0.35f, 0 }; break;
            default: break;
        }
        safe->commit();
    });
}

void FieldView::tick()
{
    const fly::Vec3 cur { proc.tele.x.load(), proc.tele.y.load(), proc.tele.z.load() };
    const auto a = toScreen (lastPos), b = toScreen (cur);
    if (a.getDistanceFrom (b) > 0.6f)
    {
        heading = std::atan2 (b.x - a.x, a.y - b.y);
        if (a.getDistanceFrom (b) > scale * 0.5f) trail.clear();   // teleport (loop restart)
        trail.add (cur);
        while (trail.size() > 40) trail.remove (0);
        lastPos = cur;
    }
    repaint();
}

void FieldView::paint (Graphics& g)
{
    const float s = uiScale;
    g.setColour (Pal::panel);
    g.fillRoundedRectangle (field, 8.0f * s);
    g.setColour (Pal::panelEdge);
    g.drawRoundedRectangle (field.reduced (0.5f), 8.0f * s, 1.0f);
    g.saveState();
    g.reduceClipRegion (field.reduced (1.0f).toNearestInt());

    const float F = proc.getParam (ParamIDs::distance);

    // floor + distance rings (circles on the tilted floor)
    const AffineTransform floorXf (scale, skew * scale, centre.x, 0.0f, -scale * squash, centre.y);
    {
        Path fl; fl.addEllipse (-1, -1, 2, 2);
        ColourGradient grad (Pal::blue.withAlpha (0.08f), centre.x, centre.y - scale * squash, Pal::blue.withAlpha (0.015f), centre.x, centre.y + scale * squash, false);
        g.setGradientFill (grad);
        g.fillPath (fl, floorXf);
    }
    g.setFont (uiFont (10.0f * s, true));
    for (int i = 1; i <= 4; ++i)
    {
        const float r = (float) i * 0.25f;
        Path ring; ring.addEllipse (-r, -r, 2 * r, 2 * r);
        g.setColour (i == 4 ? Pal::faint : Pal::faint.withAlpha (0.55f));
        g.strokePath (ring, PathStrokeType (i == 4 ? 1.2f : 0.8f), floorXf);
        const float m = F * r;
        g.setColour (Pal::dim.withAlpha (0.8f));
        const auto lp = toScreen ({ r * 0.7071f, -r * 0.7071f, 0 }, false);
        g.drawText (m < 10 ? String (m, 1) + " m" : String (roundToInt (m)) + " m", Rectangle<float> (lp.x + 3 * s, lp.y - 1 * s, 60 * s, 12 * s), Justification::centredLeft);
    }
    g.setColour (Pal::faint.withAlpha (0.4f));
    g.drawLine ({ toScreen ({ -1, 0, 0 }, false), toScreen ({ 1, 0, 0 }, false) }, 0.7f);
    g.drawLine ({ toScreen ({ 0, -1, 0 }, false), toScreen ({ 0, 1, 0 }, false) }, 0.7f);
    g.setColour (Pal::dim);
    {
        const auto fr = toScreen ({ 0, 1, 0 }, false), bk = toScreen ({ 0, -1, 0 }, false);
        const auto lf = toScreen ({ -1, 0, 0 }, false), rt = toScreen ({ 1, 0, 0 }, false);
        g.drawText ("FRONT", Rectangle<float> (fr.x - 40 * s, fr.y - 14 * s, 80 * s, 12 * s), Justification::centred);
        g.drawText ("BEHIND", Rectangle<float> (bk.x - 40 * s, bk.y + 2 * s, 80 * s, 12 * s), Justification::centred);
        g.drawText ("LEFT", Rectangle<float> (lf.x - 36 * s, lf.y - 6 * s, 32 * s, 12 * s), Justification::centredRight);
        g.drawText ("RIGHT", Rectangle<float> (rt.x + 4 * s, rt.y - 6 * s, 40 * s, 12 * s), Justification::centredLeft);
    }

    // path: ground shadow then the real (elevated) path
    bool anyHeight = false;
    for (int i = 0; i < pts.count; ++i) anyHeight |= pts.p[(size_t) i].z > 0.001f;
    auto makePath = [&] (bool elevated)
    {
        Path p;
        for (int i = 0; i <= fly::tableSize; i += 4)
        {
            const auto q = toScreen (table.pos[(size_t) i], elevated);
            if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
        }
        if (table.closed) p.closeSubPath();
        return p;
    };
    if (anyHeight)
    {
        Path shadow = makePath (false);
        Path dashed;
        const float dl[] = { 4.0f * s, 4.0f * s };
        PathStrokeType (1.0f).createDashedStroke (dashed, shadow, dl, 2);
        g.setColour (Pal::dim.withAlpha (0.45f));
        g.fillPath (dashed);
    }
    {
        Path p = makePath (true);
        g.setColour (Pal::blue.withAlpha (0.18f));
        g.strokePath (p, PathStrokeType (8.0f * s, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Pal::blue);
        g.strokePath (p, PathStrokeType (2.2f * s, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // direction chevrons
    const bool rev = proc.getParam (ParamIDs::reverse) > 0.5f;
    for (int k = 1; k < 8; ++k)
    {
        const float u = (float) k / 8.0f;
        const auto a = toScreen (table.at (u - 0.004f)), b = toScreen (table.at (u + 0.004f));
        float ang = std::atan2 (b.x - a.x, a.y - b.y);
        if (rev) ang += MathConstants<float>::pi;
        Path ch;
        ch.startNewSubPath (-4.5f * s, 3.5f * s); ch.lineTo (0, -2.5f * s); ch.lineTo (4.5f * s, 3.5f * s);
        g.setColour (Pal::text.withAlpha (0.65f));
        g.strokePath (ch, PathStrokeType (1.6f * s, PathStrokeType::mitered, PathStrokeType::rounded),
                      AffineTransform::rotation (ang).translated (toScreen (table.at (u))));
    }

    // listener
    {
        const float hr = 9.0f * s;
        g.setColour (Pal::red.withAlpha (0.25f));
        g.fillEllipse (centre.x - hr * 2, centre.y - hr * 2, hr * 4, hr * 4);
        Path nose; nose.addTriangle (centre.x - 4 * s, centre.y - hr + 2 * s, centre.x + 4 * s, centre.y - hr + 2 * s, centre.x, centre.y - hr - 6 * s);
        g.setColour (Pal::red);
        g.fillPath (nose);
        g.fillEllipse (centre.x - hr - 3 * s, centre.y - 3.5f * s, 5 * s, 7 * s);
        g.fillEllipse (centre.x + hr - 2 * s, centre.y - 3.5f * s, 5 * s, 7 * s);
        g.fillEllipse (centre.x - hr, centre.y - hr, hr * 2, hr * 2);
        g.setColour (Pal::panel);
        g.setFont (uiFont (8.0f * s, true));
        g.drawText ("YOU", Rectangle<float> (centre.x - hr, centre.y - hr, hr * 2, hr * 2), Justification::centred);
    }

    // points
    for (int i = 0; i < pts.count; ++i)
    {
        const auto v = pts.p[(size_t) i];
        const auto gp = toScreen (v, false), ep = toScreen (v, true);
        const bool hot = i == hoverIndex || i == dragIndex;
        Colour col = Pal::gold;
        if (! pts.closed && i == 0) col = Pal::green;
        if (! pts.closed && i == pts.count - 1) col = Pal::red;
        if (v.z > 0.005f)
        {
            g.setColour (Pal::dim.withAlpha (0.6f));
            g.drawLine (gp.x, gp.y, ep.x, ep.y, 1.0f);
            Path d; d.addTriangle (ep.x - 6 * s, ep.y, ep.x + 6 * s, ep.y, ep.x, ep.y - 7 * s);
            d.addTriangle (ep.x - 6 * s, ep.y, ep.x + 6 * s, ep.y, ep.x, ep.y + 7 * s);
            g.setColour (col.withAlpha (hot ? 1.0f : 0.85f));
            g.fillPath (d);
            const float hm = fly::sourceBaseHeight + v.z * F;
            g.setColour (Pal::text);
            g.setFont (uiFont (10.0f * s, true));
            g.drawText ((hm < 10 ? String (hm, 1) : String (roundToInt (hm))) + " m up", Rectangle<float> (ep.x + 8 * s, ep.y - 6 * s, 70 * s, 12 * s), Justification::centredLeft);
        }
        const float r = (hot ? 7.5f : 6.0f) * s;
        g.setColour (Colours::black.withAlpha (0.5f));
        g.fillEllipse (gp.x - r, gp.y - r + 1.5f, r * 2, r * 2);
        g.setColour (col);
        g.fillEllipse (gp.x - r, gp.y - r, r * 2, r * 2);
        g.setColour (Pal::badge);
        g.setFont (uiFont (9.0f * s, true));
        g.drawText (String (i + 1), Rectangle<float> (gp.x - r, gp.y - r, r * 2, r * 2), Justification::centred);
        if (! pts.closed && (i == 0 || i == pts.count - 1))
        {
            g.setColour (col);
            g.setFont (uiFont (10.0f * s, true));
            g.drawText (i == 0 ? (rev ? "END" : "START") : (rev ? "START" : "END"), Rectangle<float> (gp.x + r + 3 * s, gp.y - 6 * s, 60 * s, 12 * s), Justification::centredLeft);
        }
    }

    // trail + vehicle
    const float time = (float) Time::getMillisecondCounterHiRes() * 0.001f;
    for (int i = 0; i < trail.size(); ++i)
    {
        const float a = (float) (i + 1) / (float) trail.size();
        const auto q = toScreen (trail[i]);
        const float r = (1.5f + 3.0f * a) * s;
        g.setColour (Pal::gold.withAlpha (0.45f * a));
        g.fillEllipse (q.x - r, q.y - r, r * 2, r * 2);
    }
    {
        const fly::Vec3 cur { proc.tele.x.load(), proc.tele.y.load(), proc.tele.z.load() };
        const float size = (26.0f + 18.0f * cur.z) * s;
        const auto gp = toScreen (cur, false);
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillEllipse (gp.x - size * 0.35f, gp.y - size * 0.15f, size * 0.7f, size * 0.3f);
        drawVehicle (g, proc.vehicle.load(), toScreen (cur), size, heading, time);
    }

    // readouts
    {
        auto box = Rectangle<float> (field.getX() + 12 * s, field.getY() + 10 * s, 150 * s, 74 * s);
        g.setColour (Pal::badge.withAlpha (0.8f));
        g.fillRoundedRectangle (box, 6 * s);
        auto line = [&] (int row, const String& label, const String& value, Colour vc)
        {
            auto r = Rectangle<float> (box.getX() + 10 * s, box.getY() + 6 * s + (float) row * 15.5f * s, box.getWidth() - 20 * s, 15 * s);
            g.setFont (uiFont (10.0f * s, true)); g.setColour (Pal::dim);
            g.drawText (label, r, Justification::centredLeft);
            g.setFont (uiFont (12.0f * s, true)); g.setColour (vc);
            g.drawText (value, r, Justification::centredRight);
        };
        const float semis = proc.tele.semis.load();
        line (0, "SPEED", String (roundToInt (proc.tele.kmh.load())) + " km/h", Pal::text);
        line (1, "PITCH", (semis >= 0.05f ? "+" : "") + String (semis, 2) + " st", Pal::gold);
        const float d = proc.tele.dist.load();
        line (2, "DISTANCE", (d < 10 ? String (d, 1) : String (roundToInt (d))) + " m", Pal::text);
        line (3, "TIME", String (proc.tele.elapsed.load(), 2) + " / " + String (proc.tele.passSec.load(), 2) + " s", Pal::dim);
    }

    g.setColour (Pal::dim.withAlpha (0.8f));
    g.setFont (uiFont (10.0f * s));
    g.drawText ("Drag points  |  Shift+drag, wheel or the diamond = height  |  Double-click = add / remove point  |  Right-click = path options",
                field.reduced (10 * s, 6 * s), Justification::bottomLeft);
    g.restoreState();
}

//==============================================================================
void TimelineView::tick (const fly::PathTable& tbl)
{
    const auto prm = proc.currentParams();
    passSec = fly::passSeconds (prm, proc.tele.bpm.load(), 4, 4);
    if (prm.sync) passSec = proc.tele.passSec.load();
    mode = prm.mode;
    fly::predictPass (tbl, prm, passSec, count, semis.data(), level.data(), us.data());
    cursor = proc.tele.t.load();
    repaint();
}

void TimelineView::paint (Graphics& g)
{
    const float s = uiScale;
    auto b = getLocalBounds().toFloat();
    g.setColour (Pal::panel);
    g.fillRoundedRectangle (b, 8.0f * s);
    g.setColour (Pal::panelEdge);
    g.drawRoundedRectangle (b.reduced (0.5f), 8.0f * s, 1.0f);

    auto labels = b.removeFromLeft (86 * s).reduced (10 * s, 8 * s);
    g.setFont (uiFont (10.5f * s, true));
    g.setColour (Pal::gold);
    g.drawText ("PITCH", labels.removeFromTop (labels.getHeight() * 0.5f), Justification::centredLeft);
    g.setColour (Pal::blue);
    g.drawText ("LEVEL", labels, Justification::centredLeft);

    auto plot = b.reduced (8 * s, 8 * s).withTrimmedBottom (12 * s);
    float maxAbs = 0.5f;
    for (auto v : semis) maxAbs = jmax (maxAbs, std::abs (v));
    const float range = std::ceil (maxAbs * 1.15f * 2.0f) / 2.0f;

    auto xAt = [&] (int i) { return plot.getX() + plot.getWidth() * (float) i / (float) (count - 1); };
    // level fill (-48..+6 dB)
    {
        Path p;
        p.startNewSubPath (plot.getX(), plot.getBottom());
        for (int i = 0; i < count; ++i)
            p.lineTo (xAt (i), plot.getBottom() - plot.getHeight() * jlimit (0.0f, 1.0f, (level[(size_t) i] + 48.0f) / 54.0f));
        p.lineTo (plot.getRight(), plot.getBottom());
        p.closeSubPath();
        g.setColour (Pal::blue.withAlpha (0.18f));
        g.fillPath (p);
    }
    // zero line + pitch
    const float zy = plot.getCentreY();
    g.setColour (Pal::faint);
    g.drawHorizontalLine ((int) zy, plot.getX(), plot.getRight());
    {
        Path p;
        for (int i = 0; i < count; ++i)
        {
            const float y = zy - plot.getHeight() * 0.5f * semis[(size_t) i] / range;
            if (i == 0) p.startNewSubPath (xAt (i), y); else p.lineTo (xAt (i), y);
        }
        g.setColour (Pal::gold);
        g.strokePath (p, PathStrokeType (1.8f * s));
    }
    // closest point
    int best = 0;
    for (int i = 1; i < count; ++i) if (level[(size_t) i] > level[(size_t) best]) best = i;
    if (mode != 3)
    {
        const float x = xAt (best);
        g.setColour (Pal::gold.withAlpha (0.6f));
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        g.setFont (uiFont (9.5f * s, true));
        g.drawText ("CLOSEST", Rectangle<float> (x + 3 * s, plot.getY(), 60 * s, 11 * s), Justification::centredLeft);
    }
    // cursor
    {
        const float x = plot.getX() + plot.getWidth() * jlimit (0.0f, 1.0f, cursor);
        g.setColour (Colours::white);
        g.drawLine (x, plot.getY() - 2 * s, x, plot.getBottom() + 2 * s, 1.6f * s);
    }
    g.setColour (Pal::dim);
    g.setFont (uiFont (9.5f * s, true));
    auto foot = Rectangle<float> (plot.getX(), plot.getBottom() + 1 * s, plot.getWidth(), 11 * s);
    const float period = passSec * (mode == 1 ? 2.0f : 1.0f);
    g.drawText (mode == 3 ? "POSITION 0 %" : "0 s", foot, Justification::centredLeft);
    g.drawText (mode == 3 ? "100 %" : String (period, 2) + " s", foot, Justification::centredRight);
    g.drawText ("+/- " + String (range, 1) + " st", foot, Justification::centred);
}

//==============================================================================
void FlyButton::paintButton (Graphics& g, bool over, bool down)
{
    auto b = getLocalBounds().toFloat();
    const float d = jmin (b.getWidth(), b.getHeight()) * 0.78f;
    auto c = Rectangle<float> (d, d).withCentre (b.getCentre().translated (0, -4 * uiScale));
    const bool on = isEnabled();
    if (flash > 0.01f)
    {
        g.setColour (Pal::gold.withAlpha (0.35f * flash));
        g.fillEllipse (c.expanded (8 * uiScale * flash));
    }
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillEllipse (c.translated (0, 2 * uiScale));
    ColourGradient gr (on ? Pal::gold.brighter (over ? 0.25f : 0.0f) : Pal::faint, c.getCentreX(), c.getY(),
                       on ? Pal::gold.darker (down ? 0.9f : 0.5f) : Pal::knobBottom, c.getCentreX(), c.getBottom(), false);
    g.setGradientFill (gr);
    g.fillEllipse (c);
    g.setColour (on ? Pal::badge : Pal::dim);
    g.setFont (uiFont (d * (label.length() > 5 ? 0.17f : 0.22f), true));
    g.drawText (label, c, Justification::centred);
    g.setColour (Pal::text);
    g.setFont (uiFont (11.0f * uiScale, true));
    g.drawText ("TRIGGER", b.removeFromBottom (14 * uiScale), Justification::centred);
}

//==============================================================================
FlybyEditor::FlybyEditor (FlybyProcessor& p)
    : AudioProcessorEditor (p), proc (p),
      distance (p, ParamIDs::distance, "DISTANCE", "2m", "300m"),
      gain (p, ParamIDs::gain, "GAIN", "0", "200"),
      pan (p, ParamIDs::pan, "PAN", "0", "200"),
      pitch (p, ParamIDs::pitch, "PITCH", "-100", "300", true),
      air (p, ParamIDs::air, "AIR DAMP", "0", "200"),
      ground (p, ParamIDs::ground, "GROUND", "0", "100"),
      width (p, ParamIDs::width, "WIDTH", "POINT", "WIDE"),
      reverb (p, ParamIDs::reverb, "REVERB", "OFF", "+6"),
      revTime (p, ParamIDs::revTime, "REV TIME", "0.2", "10s"),
      revBright (p, ParamIDs::revBright, "BRIGHTNESS", "DARK", "BRIGHT"),
      spin (p, ParamIDs::spin, "SPIN", "0", "100"),
      spinRate (p, ParamIDs::spinRate, "SPIN RATE", "0.1", "12Hz"),
      mix (p, ParamIDs::mix, "MIX", "DRY", "WET"),
      output (p, ParamIDs::output, "OUTPUT", "-24", "+12"),
      lengthSec (p, ParamIDs::lengthSec, "LENGTH", "0.1", "60s"),
      center (p, ParamIDs::center, "CENTER", "EARLY", "LATE"),
      phase (p, ParamIDs::phase, "PHASE", "-50", "+50"),
      threshold (p, ParamIDs::threshold, "THRESHOLD", "-60", "0dB"),
      position (p, ParamIDs::position, "POSITION", "START", "END"),
      fieldView (p), timeline (p)
{
    setLookAndFeel (&look);

    for (auto* k : { &distance, &gain, &pan, &pitch, &air, &ground, &width, &reverb, &revTime, &revBright, &spin, &spinRate,
                     &mix, &output, &lengthSec, &center, &phase, &threshold, &position })
        addAndMakeVisible (*k);

    distance.slider.setTooltip ("Size of the field: the outer ring in metres. Bigger = faster movement and stronger Doppler");
    gain.slider.setTooltip ("How much the level drops with distance. 100 % = real life, 0 % = no level change");
    pan.slider.setTooltip ("Stereo movement and ear-time differences. 0 % = mono, 200 % = hard panning");
    pitch.slider.setTooltip ("Doppler pitch shift. 100 % = real physics, 0 % = none, negative = reverse Doppler");
    air.slider.setTooltip ("High-frequency loss with distance");
    ground.slider.setTooltip ("Reflection off the ground: the classic jet / car 'whoosh' flanging");
    width.slider.setTooltip ("0 % = the sound is one point (realistic). Higher keeps some of the stereo width");
    spin.slider.setTooltip ("Spins the source like a Leslie horn: level, brightness and pitch wobble");
    center.slider.setTooltip ("When the middle of the path is reached. Below 50 % = fast approach, above = fast leaving");
    phase.slider.setTooltip ("Shifts the loop so the closest point lands where you want (e.g. on the drop)");
    position.slider.setTooltip ("Manual mode: the place on the path. Automate this in REAPER to draw the movement");
    threshold.slider.setTooltip ("Audio trigger: a pass starts when the input gets louder than this");

    modeBox.addItemList ({ "MODE: LOOP", "MODE: PING-PONG", "MODE: ONE-SHOT", "MODE: MANUAL (POSITION)" }, 1);
    modeAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, ParamIDs::mode, modeBox);
    {
        StringArray names;
        for (auto* n : fly::syncNames) names.add (n);
        syncLenBox.addItemList (names, 1);
    }
    syncLenAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, ParamIDs::syncLen, syncLenBox);
    triggerBox.addItemList ({ "MIDI NOTE", "AUDIO LEVEL", "BUTTON ONLY" }, 1);
    triggerAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, ParamIDs::trigger, triggerBox);
    syncAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, ParamIDs::sync, syncToggle);
    reverseAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, ParamIDs::reverse, reverseToggle);
    keepAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, ParamIDs::keepInTime, keepToggle);
    keepToggle.setTooltip ("On: the sound is exactly on the beat when the source is closest to you. Off: real sound travel delay");
    triggerBox.setTooltip ("What starts a pass in One-Shot mode (and restarts free-running loops)");
    for (Component* c : { (Component*) &modeBox, (Component*) &syncLenBox, (Component*) &triggerBox, (Component*) &syncToggle,
                          (Component*) &reverseToggle, (Component*) &keepToggle, (Component*) &flyButton, (Component*) &presetBox,
                          (Component*) &vehicleBox, (Component*) &prevPreset, (Component*) &nextPreset, (Component*) &fieldView, (Component*) &timeline })
        addAndMakeVisible (c);

    flyButton.onClick = [this] { proc.fireTrigger(); flyButton.flash = 1.0f; };

    // presets
    refreshPresetBox();
    presetBox.onChange = [this] { const int id = presetBox.getSelectedId(); if (id > 0 && id - 1 != proc.currentPreset) loadPreset (id - 1); };
    prevPreset.onClick = [this] { const int n = proc.getNumPrograms(); loadPreset ((proc.currentPreset + n - 1) % n); };
    nextPreset.onClick = [this] { loadPreset ((proc.currentPreset + 1) % proc.getNumPrograms()); };

    for (int i = 0; i < fly::vehicleCount; ++i) vehicleBox.addItem (String ("ICON: ") + String (fly::vehicleNames[i]).toUpperCase(), i + 1);
    vehicleBox.setSelectedId (proc.vehicle.load() + 1, dontSendNotification);
    vehicleBox.onChange = [this] { proc.vehicle = jmax (0, vehicleBox.getSelectedId() - 1); };

    const int savedW = jlimit (940, 2600, proc.editorWidth), savedH = jlimit (640, 1800, proc.editorHeight);
    setResizable (true, true);
    setResizeLimits (940, 640, 2600, 1800);
    setSize (savedW, savedH);
    startTimerHz (30);
}

FlybyEditor::~FlybyEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void FlybyEditor::refreshPresetBox()
{
    presetBox.clear (dontSendNotification);
    const auto& all = fly::factoryPresets();
    for (int i = 0; i < (int) all.size(); ++i)
    {
        const String n (all[(size_t) i].name);
        if (i == 1) presetBox.addSectionHeading ("REALISTIC");
        if (n.startsWith ("Music") && ! String (all[(size_t) i - 1].name).startsWith ("Music")) presetBox.addSectionHeading ("MUSIC");
        if (n.startsWith ("Spinner (Leslie fast)")) presetBox.addSectionHeading ("CREATIVE");
        presetBox.addItem (n, i + 1);
    }
    presetBox.setSelectedId (proc.currentPreset + 1, dontSendNotification);
    lastPreset = proc.currentPreset;
}

void FlybyEditor::loadPreset (int index)
{
    proc.setCurrentProgram (index);
    presetBox.setSelectedId (index + 1, dontSendNotification);
    vehicleBox.setSelectedId (proc.vehicle.load() + 1, dontSendNotification);
    lastPreset = index;
    fieldView.syncFromProcessor();
}

void FlybyEditor::timerCallback()
{
    if (proc.currentPreset != lastPreset)
    {
        lastPreset = proc.currentPreset;
        presetBox.setSelectedId (lastPreset + 1, dontSendNotification);
        vehicleBox.setSelectedId (proc.vehicle.load() + 1, dontSendNotification);
    }
    if (vehicleBox.getSelectedId() != proc.vehicle.load() + 1) vehicleBox.setSelectedId (proc.vehicle.load() + 1, dontSendNotification);

    fieldView.syncFromProcessor();
    fieldView.tick();
    fieldView.repaint();
    timeline.tick (fieldView.getTable());

    const int mode = (int) proc.getParam (ParamIDs::mode);
    const bool sync = proc.getParam (ParamIDs::sync) > 0.5f;
    const int trig = (int) proc.getParam (ParamIDs::trigger);
    lengthSec.setAlpha (sync ? 0.3f : 1.0f);
    syncLenBox.setAlpha (sync ? 1.0f : 0.4f);
    phase.setAlpha (mode <= 1 ? 1.0f : 0.3f);
    center.setAlpha (mode == 3 ? 0.3f : 1.0f);
    threshold.setAlpha (trig == 1 ? 1.0f : 0.3f);
    position.setAlpha (mode == 3 ? 1.0f : 0.3f);
    const String label = mode == 2 ? "FLY!" : (mode == 3 ? "MANUAL" : "RESTART");
    flyButton.setEnabled (mode != 3);
    if (flyButton.label != label) { flyButton.label = label; flyButton.repaint(); }
    if (flyButton.flash > 0.0f) { flyButton.flash = jmax (0.0f, flyButton.flash - 0.08f); flyButton.repaint(); }
}

void FlybyEditor::paint (Graphics& g)
{
    g.setGradientFill (ColourGradient (Pal::bgTop, 0, 0, Pal::bgBottom, 0, (float) getHeight(), false));
    g.fillAll();

    auto h = headerArea.toFloat().reduced (16 * sc, 0);
    const auto logo = uiFont (24.0f * sc, true);
    const String a = "Fly", b = "by";
    const float aw = GlyphArrangement::getStringWidth (logo, a);
    g.setFont (logo);
    g.setColour (Pal::text);
    g.drawText (a, h, Justification::centredLeft);
    g.setColour (Pal::blue);
    g.drawText (b, h.withTrimmedLeft (aw), Justification::centredLeft);
    const float tx = aw + GlyphArrangement::getStringWidth (logo, b) + 12 * sc;
    g.setColour (Pal::dim);
    g.setFont (uiFont (11.0f * sc, true));
    g.drawText ("DOPPLER MOTION", h.withTrimmedLeft (tx), Justification::centredLeft);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.drawHorizontalLine (headerArea.getBottom(), 0.0f, (float) getWidth());

    // motion panel
    g.setColour (Pal::panel.withAlpha (0.55f));
    g.fillRoundedRectangle (panelArea.toFloat(), 8 * sc);
    g.setColour (Pal::panelEdge);
    g.drawRoundedRectangle (panelArea.toFloat().reduced (0.5f), 8 * sc, 1.0f);
    g.setFont (uiFont (12.0f * sc, true));
    g.setColour (Pal::blue);
    g.drawText ("MOTION", motionTitle, Justification::centredLeft);
    g.drawText ("TRIGGER", triggerTitle, Justification::centredLeft);

    // knob rows
    for (auto [r, t] : { std::pair<Rectangle<int>, const char*> { rowA, "REALITY" }, { rowB, "SPACE  |  SPIN  |  OUTPUT" } })
    {
        g.setColour (Pal::panel.withAlpha (0.35f));
        g.fillRoundedRectangle (r.toFloat(), 8 * sc);
        g.setColour (Pal::dim);
        g.setFont (uiFont (10.5f * sc, true));
        g.drawText (t, r.toFloat().reduced (12 * sc, 5 * sc).removeFromTop (12 * sc), Justification::centredLeft);
    }
}

void FlybyEditor::resized()
{
    proc.editorWidth = getWidth();
    proc.editorHeight = getHeight();
    sc = jmin (getWidth() / 1180.0f, getHeight() / 800.0f);
    look.uiScale = sc;
    fieldView.uiScale = sc;
    timeline.uiScale = sc;
    flyButton.uiScale = sc;
    auto S = [this] (float v) { return (int) std::round (v * sc); };

    auto r = getLocalBounds();
    headerArea = r.removeFromTop (S (48));
    {
        auto hd = headerArea.reduced (S (14), S (10));
        vehicleBox.setBounds (hd.removeFromRight (S (180)));
        hd.removeFromRight (S (14));
        nextPreset.setBounds (hd.removeFromRight (S (30)));
        hd.removeFromRight (S (4));
        presetBox.setBounds (hd.removeFromRight (S (290)));
        hd.removeFromRight (S (4));
        prevPreset.setBounds (hd.removeFromRight (S (30)));
    }

    r.reduce (S (10), S (6));
    rowB = r.removeFromBottom (S (110));
    r.removeFromBottom (S (6));
    rowA = r.removeFromBottom (S (110));
    r.removeFromBottom (S (8));

    auto layoutRow = [&] (Rectangle<int> row, std::initializer_list<Knob*> knobs)
    {
        auto k = row.reduced (S (12), S (4));
        k.removeFromTop (S (12));
        const int w = k.getWidth() / (int) knobs.size();
        for (auto* kn : knobs) kn->setBounds (k.removeFromLeft (w).reduced (S (6), S (2)));
    };
    layoutRow (rowA, { &distance, &gain, &pan, &pitch, &air, &ground, &width });
    layoutRow (rowB, { &reverb, &revTime, &revBright, &spin, &spinRate, &mix, &output });
    pitch.setBounds (pitch.getBounds().expanded (S (6), S (2)));

    panelArea = r.removeFromRight (S (300));
    r.removeFromRight (S (8));
    timeline.setBounds (r.removeFromBottom (S (86)));
    r.removeFromBottom (S (6));
    fieldView.setBounds (r);

    auto pa = panelArea.reduced (S (12), S (8));
    const int spare = jmax (0, pa.getHeight() - S (400)) / 6;
    motionTitle = pa.removeFromTop (S (20));
    modeBox.setBounds (pa.removeFromTop (S (30)));
    pa.removeFromTop (S (8) + spare);
    {
        auto row = pa.removeFromTop (S (28));
        syncToggle.setBounds (row.removeFromLeft (S (128)));
        syncLenBox.setBounds (row);
    }
    pa.removeFromTop (S (6) + spare);
    {
        auto row = pa.removeFromTop (S (96));
        const int w = row.getWidth() / 3;
        lengthSec.setBounds (row.removeFromLeft (w));
        center.setBounds (row.removeFromLeft (w));
        phase.setBounds (row);
    }
    pa.removeFromTop (S (6) + spare);
    triggerTitle = pa.removeFromTop (S (20));
    triggerBox.setBounds (pa.removeFromTop (S (28)));
    pa.removeFromTop (S (6) + spare);
    {
        auto row = pa.removeFromTop (S (100));
        const int w = row.getWidth() / 3;
        threshold.setBounds (row.removeFromLeft (w));
        position.setBounds (row.removeFromLeft (w));
        flyButton.setBounds (row);
    }
    pa.removeFromTop (S (8) + spare);
    reverseToggle.setBounds (pa.removeFromTop (S (24)));
    pa.removeFromTop (S (4));
    keepToggle.setBounds (pa.removeFromTop (S (24)));
}
