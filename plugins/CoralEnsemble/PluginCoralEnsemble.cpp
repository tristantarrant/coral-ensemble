/*
 * Coral Ensemble - BOSS CE-2 Chorus Ensemble emulation
 * DPF plugin wrapper around the faust-generated DSP class.
 *
 * Copyright (C) 2026 Dataforte
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DistrhoPlugin.hpp"

#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <vector>

// Faust/dsp2cc generated DSP (binds controls by PortIndex)
#include "../../src/ce_glue.h"
#include "../../src/coralensemble.h"
#include "../../src/coralensemble.cc"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------
// Presets: documented CE-2 settings (see README). Program 0 equals the
// parameter defaults.

struct CeProgram {
    const char* name;
    float rate, depth, mix;
    float model;      // 0 = CE-2 (guitar), 1 = CE-2B (bass)
};

static const CeProgram kPrograms[] = {
    { "CE-2 noon - classic 80s clean chorus",                       5.0f, 5.0f, 10.0f, 0.0f },
    { "Smiths clean shimmer (Marr-style, rate 10-11, depth 1-2)",   3.5f, 6.5f, 10.0f, 0.0f },
    { "TSOL / SoCal death rock (rate 9 o'clock, depth near noon)",  2.0f, 4.8f, 10.0f, 0.0f },
    { "Max warp, both knobs full (gentle warbling vibrato)",       10.0f,10.0f, 10.0f, 0.0f },
    { "Bass: low end preserved (CE-2B)",                            3.0f, 5.0f,  6.5f, 1.0f },
};
static constexpr uint32_t kProgramCount = sizeof(kPrograms) / sizeof(kPrograms[0]);

// --------------------------------------------------------------------

class PluginCoralEnsemble : public Plugin
{
public:
    PluginCoralEnsemble()
        : Plugin(kParamCount, kProgramCount, 0),
          fDsp(coralensemble::plugin())
    {
        for (uint32_t i = 0; i < kParamBypass; ++i)
            fParams[i] = 0.0f;
        fParams[kParamBypass] = 0.0f;
        setProgramValues(0);

        for (uint32_t i = 0; i < kParamBypass; ++i)
            fDsp->connect_ports(i, &fParams[i], fDsp);

        fDsp->set_samplerate(static_cast<uint32_t>(getSampleRate()), fDsp);
        fDry.resize(getBufferSize());
        fFadeStep = 1.0f / (0.010f * static_cast<float>(getSampleRate()));
    }

    ~PluginCoralEnsemble() override
    {
        if (fDsp->activate_plugin != nullptr)
            fDsp->activate_plugin(false, fDsp);
        fDsp->delete_instance(fDsp);
    }

protected:
    const char* getLabel() const override { return "coralensemble"; }
    const char* getDescription() const override
    {
        return "Emulation of the 1979 BOSS CE-2 Chorus Ensemble: BJT input buffer, "
               "inverting pre-emphasis shelving, 3rd-order Sallen-Key anti-aliasing "
               "filter, MN3007 bucket-brigade delay clocked by an MN3101 whose "
               "oscillator is swept by a TL022 triangle LFO, reconstruction filter, "
               "de-emphasis and dry/wet summing. MODEL selects the 1987 CE-2B bass "
               "component set (wet highpass 40 Hz instead of 14.6 Hz). RATE and "
               "DEPTH are pot positions as on the pedal; MIX is the CE-2B E.LEVEL.";
    }
    const char* getMaker() const override { return "Dataforte"; }
    const char* getHomePage() const override { return "https://github.com/tristantarrant/coral-ensemble"; }
    const char* getLicense() const override { return "GPL-2.0-or-later"; }
    uint32_t getVersion() const override { return d_version(1, 0, 0); }
    int64_t getUniqueId() const override { return d_cconst('C', 'o', 'E', 'n'); }

    void initAudioPort(const bool input, const uint32_t index, AudioPort& port) override
    {
        port.groupId = kPortGroupMono;
        Plugin::initAudioPort(input, index, port);
        // keep the symbols of the pre-DPF plugin: pedalboards store them
        port.symbol = input ? "in" : "out";
        port.name   = input ? "In" : "Out";
    }

    void initParameter(const uint32_t index, Parameter& p) override
    {
        p.hints = kParameterIsAutomatable;
        p.ranges.min = 0.0f;
        p.ranges.max = 10.0f;

        switch (index)
        {
        case kParamRate:
            p.name = "RATE"; p.symbol = "rate";
            p.ranges.def = kPrograms[0].rate;
            break;
        case kParamDepth:
            p.name = "DEPTH"; p.symbol = "depth";
            p.ranges.def = kPrograms[0].depth;
            break;
        case kParamMix:
            p.name = "MIX"; p.symbol = "mix";
            p.ranges.def = kPrograms[0].mix;
            break;
        case kParamModel:
            p.hints |= kParameterIsInteger;
            p.name = "MODEL"; p.symbol = "model";
            p.ranges.max = 1.0f;
            p.ranges.def = 0.0f;
            setEnum(p, { "Guitar", "Bass" });
            break;
        case kParamBypass:
            p.initDesignation(kParameterDesignationBypass);
            break;
        }
    }

    void initProgramName(const uint32_t index, String& name) override
    {
        if (index < kProgramCount)
            name = kPrograms[index].name;
    }

    void loadProgram(const uint32_t index) override
    {
        if (index < kProgramCount)
            setProgramValues(index);
    }

    float getParameterValue(const uint32_t index) const override
    {
        return index < kParamCount ? fParams[index] : 0.0f;
    }

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index < kParamCount)
            fParams[index] = value;
    }

    void activate() override
    {
        if (fDsp->activate_plugin != nullptr)
            fDsp->activate_plugin(true, fDsp);
        fWet = fParams[kParamBypass] > 0.5f ? 0.0f : 1.0f;
    }

    void deactivate() override
    {
        if (fDsp->activate_plugin != nullptr)
            fDsp->activate_plugin(false, fDsp);
    }

    void bufferSizeChanged(const uint32_t newBufferSize) override
    {
        fDry.resize(newBufferSize);
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        fDsp->set_samplerate(static_cast<uint32_t>(newSampleRate), fDsp);
        fFadeStep = 1.0f / (0.010f * static_cast<float>(newSampleRate));
    }

    void run(const float** inputs, float** outputs, const uint32_t frames) override
    {
        AVOIDDENORMALS();

        const float* const in = inputs[0];
        float* const out = outputs[0];
        if (frames == 0)
            return;

        const float target = fParams[kParamBypass] > 0.5f ? 0.0f : 1.0f;
        const bool fading = fWet != target;

        // keep the DSP running while bypassed so that re-engaging is seamless
        if (fading || target == 0.0f)
            std::memcpy(fDry.data(), in, sizeof(float) * frames);

        fDsp->mono_audio(static_cast<int>(frames), const_cast<float*>(in), out, fDsp);

        if (!fading && target == 1.0f)
            return;
        if (!fading)
        {
            std::memcpy(out, fDry.data(), sizeof(float) * frames);
            return;
        }
        for (uint32_t i = 0; i < frames; ++i)
        {
            if (fWet < target)
                fWet = std::min(target, fWet + fFadeStep);
            else if (fWet > target)
                fWet = std::max(target, fWet - fFadeStep);
            out[i] = fDry[i] + fWet * (out[i] - fDry[i]);
        }
    }

private:
    PluginLV2* const fDsp;
    float fParams[kParamCount];
    std::vector<float> fDry;
    float fWet = 1.0f;
    float fFadeStep = 0.002f;

    void setProgramValues(const uint32_t index)
    {
        fParams[kParamRate]  = kPrograms[index].rate;
        fParams[kParamDepth] = kPrograms[index].depth;
        fParams[kParamMix]   = kPrograms[index].mix;
        fParams[kParamModel] = kPrograms[index].model;
    }

    static void setEnum(Parameter& p, std::initializer_list<const char*> labels)
    {
        const uint32_t n = static_cast<uint32_t>(labels.size());
        ParameterEnumerationValue* const values = new ParameterEnumerationValue[n];
        uint32_t i = 0;
        for (const char* label : labels)
        {
            values[i].label = label;
            values[i].value = static_cast<float>(i);
            ++i;
        }
        p.enumValues.count = n;
        p.enumValues.restrictedMode = true;
        p.enumValues.values = values;
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginCoralEnsemble)
};

Plugin* createPlugin()
{
    return new PluginCoralEnsemble();
}

END_NAMESPACE_DISTRHO
