declare id "coralensemble";
declare name "Coral Ensemble";
declare category "Modulation";
declare author "Dataforte";
declare license "GPL-2.0-or-later";
declare description "Emulation of the 1979 BOSS CE-2 Chorus Ensemble: BJT input buffer, inverting pre-emphasis shelving, 3rd-order Sallen-Key anti-aliasing filter, MN3007 bucket-brigade delay (1024 stages) clocked by an MN3101 whose oscillator is modulated by a TL022 triangle LFO, reconstruction filter, de-emphasis and dry/wet summing. The 1987 CE-2B bass version (wet-path highpass 33n -> 12n) is included as a model. Controls are pot positions as on the pedal, plus a MIX (E.LEVEL) knob.";

import("stdfaust.lib");
tf = library("coralensemble_tf.lib");

// ====================================================================
//  Circuit constants
// ====================================================================

// Signal level: 1.0 full scale <-> 1 V at the pedal input and output
// (unity in/out, so the pedal's own gain structure is kept).
VIN = 1.0;

// BBD: MN3007, 1024 stages, delay = 1024/(2*f_clock).
// Measured on a CE-2 at the MN3007 clock pins (Fender3D, diystompboxes
// forum, 2011): 110 kHz with DEPTH at 0 (the sweep centre) and
// 100..128 kHz with DEPTH at noon. The clock is modulated linearly by
// the LFO voltage (charge current through R38 150k into C22 47p), so
// the delay is D_C/(1 - gamma*d) with d the normalized triangle.
K_CLOCK = 110e3;                    // Hz, clock at the sweep centre
N_BBD   = 1024.0;                   // MN3007 stages
D_C     = N_BBD/(2*K_CLOCK);        // 4.65 ms
GAMMA   = 0.246;                    // clock swing at full DEPTH (measured)

// LFO: TL022 Schmitt trigger + integrator (R30 47k, R29 33k, R32 1M,
// C19 0.1u). f = R30/(4*R29*R32*C19) = 3.5 Hz at RATE maximum (and
// measured 3.5 Hz with both pots up); the pots are 100kB linear.
// Measured range: 3.5 Hz at maximum, 0.3 Hz at minimum (anasounds
// scope measurements of a CE-2).
F_RATE_MIN = 0.3;                   // Hz
F_RATE_MAX = 3.5;                   // Hz

// BBD bucket swing limit. Estimate for a 9 V part: the MN3007 is
// specified for 0.5% THD at 0.78 Vrms in, and the buckets swing to
// roughly a third of the supply. Soft clip only; transparent at
// normal guitar levels even after the pre-emphasis treble boost.
VBBD = 2.5;                         // V (estimate)

// ====================================================================
//  Controls
// ====================================================================
// RATE and DEPTH are pot positions, 0..10 = fraction of travel
// clockwise, as on the pedal (7 o'clock .. 5 o'clock; noon = 5).
// Defaults: both at noon, the classic CE-2 setting, MIX at maximum
// (= the CE-2's fixed 50/50 mix).

rate  = hslider("rate[name:RATE]", 5.0, 0.0, 10.0, 0.1) : si.smooth(0.999);
depth = hslider("depth[name:DEPTH]", 5.0, 0.0, 10.0, 0.1) : si.smooth(0.999);

// MIX: the CE-2B's E.LEVEL (250kC wet-level divider). At 10 it is the
// stock CE-2 mix (equal dry and wet); linear in pot travel (the
// reverse-log taper is not reproduced, see README).
mix   = hslider("mix[name:MIX]", 10.0, 0.0, 10.0, 0.1) : si.smooth(0.999);

// MODEL: component set. 0 = CE-2 (guitar), 1 = CE-2B (bass): the only
// difference is the wet-path highpass cap C14, 33n vs 12n (14.6 Hz vs
// 40 Hz), which keeps the low end out of the modulated signal.
model = nentry("model[name:MODEL]", 0, 0, 1, 1);

// ====================================================================
//  LFO and clock
// ====================================================================
// The LFO is the TL022 Schmitt-trigger + integrator triangle. The RATE
// pot divides the square wave that drives the integrator, so the
// frequency is linear in pot travel (measured endpoints used here).
f_lfo = F_RATE_MIN + (F_RATE_MAX - F_RATE_MIN)*rate/10;

ph = phnode ~ _ with {
    phnode(p) = ma.decimal(p + f_lfo/ma.SR);
};

tri = 1 - abs(2*ph - 1);            // 0..1 triangle
lfo = (2*tri - 1)                   // -1..1, the integrator output
      : fi.lowpass(1, 72);          // R34 220k / C20 10n at Q5's base

// Delay time: the LFO voltage sets the clock through Q4/D2/C22, and
// delay = 1024/(2*f_clock). Depth divides the triangle (DEPTH pot).
d_s = D_C/(1 - GAMMA*(depth/10)*lfo) * ma.SR;

// ====================================================================
//  Bucket brigade delay
// ====================================================================
// Bucket swing limit (soft), then the variable delay. The BBD samples
// at its clock rate; since the anti-aliasing filter cuts at 6.6 kHz,
// well below the clock/Nyquist, a fractional-delay model is faithful
// in the audio band. 4-point Hermite interpolation keeps the wet
// signal clean while the delay sweeps.
clip(x) = x / pow(1 + pow(abs(x)/VBBD, 4), 0.25);

bbd(x) = de.fdelay4(DSIZE, d_s, clip(x)) with {
    DSIZE = int(0.008*ma.SR) + 8;   // 8 ms covers 3.7..6.2 ms sweep
};

// ====================================================================
//  Signal path
// ====================================================================
// Q1 input buffer: C1 47n into R1 1k + R2 470k (7.2 Hz), C2 470n into
// R4 47k (7.2 Hz).
inbuf = fi.highpass(1, 1/(2*ma.PI*471e3*47e-9))
      : fi.highpass(1, 1/(2*ma.PI*47e3*470e-9));

// Wet path: C8 33n + R13 100k (48 Hz) -> anti-aliasing filter ->
// BBD -> C14 33n (12n) + R15 330k (14.6 / 40 Hz) -> reconstruction.
// The two Sallen-Key filters share the same value set (mirrored).
HP_AA  = 1/(2*ma.PI*100e3*33e-9);                              // 48.2 Hz
HP_REC = 1/(2*ma.PI*330e3*select2(model, 33e-9, 12e-9));       // 14.6 / 40.2 Hz

wet(x) = x : fi.highpass(1, HP_AA) : tf.sk : bbd
           : fi.highpass(1, HP_REC) : tf.sk;

// Dry and wet into the de-emphasis summing amp: R21 and R22 are equal
// 47k, so dry and wet each reach the virtual ground at unity; MIX
// scales the wet branch (the CE-2B's E.LEVEL pot).
mixstage(x) = ysum with {
    dry  = x;
    wetx = x : wet;
    ysum = dry + m*wetx;
    m    = mix/10;
};

// Output: C7 1uF + R12 100k (1.6 Hz).
outbuf = fi.highpass(1, 1/(2*ma.PI*100e3*1e-6));

process = *(VIN) : inbuf : tf.pre : mixstage : tf.deemph : outbuf : /(VIN);
