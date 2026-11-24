#pragma once

#ifndef CE_CORALENSEMBLE_H
#define CE_CORALENSEMBLE_H


#define CEPLUGIN_URI "http://dataforte.net/plugins/coralensemble"

// Control ports in the order of the faust sliders in
// src/coralensemble.dsp (dsp2cc binds them by this enum) and of the
// DPF Parameters enum in plugins/CoralEnsemble/DistrhoPluginInfo.h.
typedef enum
{
  RATE,
  DEPTH,
  MIX,
  MODEL,
  EFFECTS_OUTPUT,
  EFFECTS_INPUT,
} PortIndex;

#endif // CE_CORALENSEMBLE_H
