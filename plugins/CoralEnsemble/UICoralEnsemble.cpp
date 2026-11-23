/*
 * Coral Ensemble - scalable cairo UI (DPF).
 *
 * The pedal is drawn by Artwork.hpp in a fixed design space and scaled
 * to the window, keeping the aspect ratio. Interaction is mapped through
 * the artwork's control table:
 *   knobs        drag up/down (Shift: fine), mouse wheel, double-click
 *                resets to the default
 *   switch       click toggles GUITAR / BASS
 *   footswitch   click toggles bypass
 *
 * Copyright (C) 2026 Dataforte
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DistrhoUI.hpp"
#include "Artwork.hpp"

#include <algorithm>

START_NAMESPACE_DISTRHO

using DGL_NAMESPACE::CairoGraphicsContext;

static_assert(static_cast<int>(ce_art::P_COUNT) == static_cast<int>(kParamCount), "artwork and plugin parameter lists differ");

class UICoralEnsemble : public UI
{
public:
    UICoralEnsemble()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT)
    {
        const double s = getScaleFactor();
        if (s != 1.0)
            setSize(static_cast<uint>(ce_art::W * s), static_cast<uint>(ce_art::H * s));
        // resizable from half to any size, aspect ratio kept
        setGeometryConstraints(static_cast<uint>(ce_art::W * 0.5 * s),
                               static_cast<uint>(ce_art::H * 0.5 * s), true, false);

        for (int i = 0; i < ce_art::P_COUNT; ++i)
            fDefaults[i] = fState.value[i];
    }

protected:
    // ---- host -> UI ------------------------------------------------
    void parameterChanged(const uint32_t index, const float value) override
    {
        if (index < static_cast<uint32_t>(ce_art::P_COUNT)) {
            fState.value[index] = value;
            repaint();
        }
    }

    // ---- drawing ---------------------------------------------------
    void onCairoDisplay(const CairoGraphicsContext& context) override
    {
        cairo_t* const cr = context.handle;
        updateTransform();

        cairo_set_source_rgb(cr, 0.12, 0.12, 0.12);
        cairo_paint(cr);

        cairo_save(cr);
        cairo_translate(cr, fOffX, fOffY);
        cairo_scale(cr, fScale, fScale);
        ce_art::draw(cr, fState);
        cairo_restore(cr);
    }

    // ---- interaction -----------------------------------------------
    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button != 1)
            return false;

        if (!ev.press) {
            if (fDragging >= 0) {
                editParameter(fDragging, false);
                fDragging = -1;
                return true;
            }
            return false;
        }

        const ce_art::Control* c = hit(ev.pos.getX(), ev.pos.getY());
        if (c == nullptr)
            return false;

        if (c->kind == ce_art::kKnob) {
            const bool dbl = c->param == fLastClickParam && ev.time - fLastClickTime < 350;
            fLastClickParam = c->param;
            fLastClickTime = ev.time;
            if (dbl) {
                setValue(c->param, fDefaults[c->param], true);
                return true;
            }
            fDragging = c->param;
            fDragY = ev.pos.getY();
            fDragValue = fState.value[c->param];
            editParameter(c->param, true);
            return true;
        }

        // toggles and footswitch
        setValue(c->param, fState.value[c->param] > 0.5f ? 0.0f : 1.0f, true);
        return true;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (fDragging < 0)
            return false;
        const ce_art::Control* c = control(fDragging);
        const double span = c->max - c->min;
        // full travel over 200 design units (Shift: 10x finer)
        double pixelsPerTravel = 200.0 * fScale;
        if (ev.mod & kModifierShift)
            pixelsPerTravel *= 10.0;
        const double dv = (fDragY - ev.pos.getY()) / pixelsPerTravel * span;
        setValue(fDragging, static_cast<float>(fDragValue + dv), false);
        return true;
    }

    bool onScroll(const ScrollEvent& ev) override
    {
        const ce_art::Control* c = hit(ev.pos.getX(), ev.pos.getY());
        if (c == nullptr || c->kind != ce_art::kKnob)
            return false;
        const double step = (c->max - c->min) / ((ev.mod & kModifierShift) ? 500.0 : 50.0);
        setValue(c->param, static_cast<float>(fState.value[c->param] + ev.delta.getY() * step), true);
        return true;
    }

private:
    ce_art::State fState;
    float fDefaults[ce_art::P_COUNT];
    double fScale = 1.0, fOffX = 0.0, fOffY = 0.0;
    int fDragging = -1;
    double fDragY = 0.0;
    double fDragValue = 0.0;
    int fLastClickParam = -1;
    uint fLastClickTime = 0;

    void updateTransform()
    {
        const double w = getWidth(), h = getHeight();
        fScale = std::min(w / ce_art::W, h / ce_art::H);
        fOffX = (w - ce_art::W * fScale) / 2.0;
        fOffY = (h - ce_art::H * fScale) / 2.0;
    }

    static const ce_art::Control* control(const int param)
    {
        for (const ce_art::Control& c : ce_art::kControls)
            if (c.param == param)
                return &c;
        return nullptr;
    }

    const ce_art::Control* hit(const double px, const double py)
    {
        updateTransform();
        const double x = (px - fOffX) / fScale;
        const double y = (py - fOffY) / fScale;
        for (const ce_art::Control& c : ce_art::kControls) {
            if (c.kind == ce_art::kToggle) {
                if (std::abs(x - c.x) <= c.w / 2 && std::abs(y - c.y) <= c.h / 2)
                    return &c;
            } else {
                const double dx = x - c.x, dy = y - c.y;
                if (dx * dx + dy * dy <= c.r * c.r)
                    return &c;
            }
        }
        return nullptr;
    }

    // set locally, notify the host (wrapping in a gesture when needed)
    void setValue(const int param, float value, const bool gesture)
    {
        const ce_art::Control* c = control(param);
        value = std::max(static_cast<float>(c->min), std::min(static_cast<float>(c->max), value));
        if (c->kind != ce_art::kKnob)
            value = value > 0.5f ? 1.0f : 0.0f;
        if (gesture)
            editParameter(param, true);
        fState.value[param] = value;
        setParameterValue(param, value);
        if (gesture)
            editParameter(param, false);
        repaint();
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UICoralEnsemble)
};

UI* createUI()
{
    return new UICoralEnsemble();
}

END_NAMESPACE_DISTRHO
