/*
 * Coral Ensemble - LV2 test harness
 *
 * Loads the plugin with the raw LV2 C API, exercises the DSP with a few
 * control/audio scenarios and checks the results against the measured
 * behaviour of the BOSS CE-2 that the model encodes:
 *   - the fixed delay at DEPTH 0 is 4.65 ms (110 kHz clock, 1024 stages);
 *   - DEPTH sweeps it over 3.7..6.2 ms;
 *   - RATE sets the LFO between 0.3 and 3.5 Hz;
 *   - the CE-2B model rolls the wet path off below ~40 Hz;
 *   - MIX scales the wet signal (at maximum: the stock 50/50 mix).
 *
 * Usage: ./ce_test /path/to/coralensemble.lv2/coralensemble_dsp.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>

#include "lv2host.h"

typedef struct _LV2_Descriptor LV2_Descriptor;
typedef void *LV2_Handle;

struct _LV2_Descriptor {
  const char *plugin_uri;
  LV2_Handle (*instantiate)(const LV2_Descriptor *, double, const char *,
                            const void *const *);
  void (*connect_port)(LV2_Handle, unsigned int, void *);
  void (*activate)(LV2_Handle);
  void (*run)(LV2_Handle, unsigned int);
  void (*deactivate)(LV2_Handle);
  void (*cleanup)(LV2_Handle);
  void *reserved;
};

/* DPF port layout: audio in, audio out, parameters, lv2:enabled */
enum { RATE, DEPTH, MIX, MODEL, NCTRL };
#define P_IN      0
#define P_OUT     1
#define P_CTRL0   2
#define P_ENABLED 6

/* model constants (must match src/coralensemble.dsp) */
#define D_C    4.6545e-3      /* 1024/(2*110 kHz) */
#define GAMMA  0.246

#define BLOCK 2048

static int failures = 0;
static const LV2_Descriptor *desc;
static float ctrl[NCTRL];
static float enabled = 1.0f;
static float in[BLOCK], out[BLOCK];

static void
check(int ok, const char *msg)
{
  printf("%s: %s\n", ok ? "PASS" : "FAIL", msg);
  if (!ok)
    ++failures;
}

static void
defaults(void)
{
  ctrl[RATE] = 5.0f; ctrl[DEPTH] = 5.0f;
  ctrl[MIX] = 10.0f; ctrl[MODEL] = 0.0f;
}

static LV2_Handle
open_plugin(double sr)
{
  LV2_Handle h = desc->instantiate(desc, sr, NULL,
                                   (const void *const *)host_get_features(BLOCK));
  if (!h)
    return NULL;
  for (int i = 0; i < NCTRL; ++i)
    desc->connect_port(h, P_CTRL0 + i, &ctrl[i]);
  enabled = 1.0f;
  desc->connect_port(h, P_ENABLED, &enabled);
  desc->connect_port(h, P_OUT, out);
  desc->connect_port(h, P_IN, in);
  desc->activate(h);
  return h;
}

static void
close_plugin(LV2_Handle h)
{
  desc->deactivate(h);
  desc->cleanup(h);
}

static double
db(double x)
{
  return 20.0 * log10(x + 1e-30);
}

/* Run a continuous-phase sine for `secs`; return the output rms over the
   last `tail` blocks (tail=1: the last block). *finite reports NaN/Inf. */
static double
sine_rms(LV2_Handle h, double sr, double freq, double amp, double secs,
         int tail, int *finite)
{
  static long phase = 0;
  int nblocks = (int)(secs * sr / BLOCK) + 1;
  double acc = 0;
  int n = 0;
  if (tail < 1)
    tail = 1;
  *finite = 1;
  for (int b = 0; b < nblocks; ++b) {
    for (int i = 0; i < BLOCK; ++i, ++phase)
      in[i] = (float)(amp * sin(2.0 * M_PI * freq * phase / sr));
    desc->run(h, BLOCK);
    if (b >= nblocks - tail) {
      for (int i = 0; i < BLOCK; ++i) {
        if (!isfinite(out[i]))
          *finite = 0;
        acc += (double)out[i] * out[i];
      }
      n += BLOCK;
    }
  }
  return sqrt(acc / n);
}

/* advance the LFO (and the delay line) with `samples` of silence */
static void
advance(LV2_Handle h, long samples)
{
  while (samples > 0) {
    int n = samples > BLOCK ? BLOCK : (int)samples;
    memset(in, 0, sizeof(float) * n);
    desc->run(h, n);
    samples -= n;
  }
}

/* Send one small impulse and return the arrival time of the wet packet
   (peak of the response after 1.2 ms; the dry impulse and the filters'
   tiny low-frequency tails are far below it). */
static double
click_delay(LV2_Handle h, double sr)
{
  memset(in, 0, sizeof(float) * BLOCK);
  in[0] = 0.2f;
  desc->run(h, BLOCK);
  const int start = (int)(1.2e-3 * sr);
  const int stop = (int)(9.0e-3 * sr);
  double best = 0;
  int bi = start;
  for (int i = start; i < stop; ++i) {
    const double a = fabs(out[i]);
    if (a > best) {
      best = a;
      bi = i;
    }
  }
  return bi / sr;
}

int
main(int argc, char **argv)
{
  if (argc < 2) {
    fprintf(stderr, "usage: %s plugin.so\n", argv[0]);
    return 1;
  }
  void *lib = dlopen(argv[1], RTLD_NOW);
  if (!lib) {
    fprintf(stderr, "cannot dlopen %s: %s\n", argv[1], dlerror());
    return 1;
  }
  const LV2_Descriptor *(*desc_fn)(unsigned int) =
    (const LV2_Descriptor * (*)(unsigned int)) dlsym(lib, "lv2_descriptor");
  desc = desc_fn ? desc_fn(0) : NULL;
  check(desc != NULL && desc->plugin_uri != NULL, "lv2_descriptor(0)");
  if (!desc)
    return 1;

  const double SR = 48000;
  int fin;
  double r, r1, r2;
  char msg[200];

  defaults();
  LV2_Handle h = open_plugin(SR);
  check(h != NULL, "instantiate @48k");
  if (!h)
    return 1;

  /* --- bypass (lv2_enabled = 0) must be exact dry ------------------ */
  enabled = 0.0f;
  sine_rms(h, SR, 1000, 0.5, 0.5, 1, &fin);
  double maxdiff = 0;
  for (int i = 0; i < BLOCK; ++i)
    maxdiff = fmax(maxdiff, fabs((double)out[i] - in[i]));
  check(maxdiff < 1e-6, "bypassed: output == input (exact dry)");
  enabled = 1.0f;
  sine_rms(h, SR, 1000, 0.5, 0.3, 1, &fin);

  /* --- defaults: sane level ---------------------------------------- */
  r = sine_rms(h, SR, 1000, 0.5, 1.0, 1, &fin);
  check(fin && r > 0.01 && r < 2.0, "defaults: finite, sensible level");
  printf("      rms(defaults @1 kHz) = %.4f (input rms %.4f)\n", r, 0.3536);

  /* --- MIX: 0 is dry only, 10 is the stock 50/50 mix --------------- */
  ctrl[DEPTH] = 0.0f;
  ctrl[MIX] = 0.0f;
  r1 = sine_rms(h, SR, 1000, 0.3, 1.0, 2, &fin);
  snprintf(msg, sizeof msg, "MIX 0: dry only, level %.2f dB vs input (%+.2f dB)",
           db(r1 / 0.2121), db(r1 / 0.2121));
  check(fin && fabs(db(r1 / 0.2121)) < 1.0, msg);

  /* notch of the static 4.65 ms comb: 1/(2*D) plus the +27 deg that the
     wet path's 48 Hz/14.6 Hz highpasses add, so ~123 Hz (the second
     notch, where the highpass phase is small, lands at ~328 Hz). */
  {
    double dryref = 0;
    ctrl[MIX] = 0.0f;
    dryref = sine_rms(h, SR, 122, 0.3, 0.7, 2, &fin);
    ctrl[MIX] = 10.0f;
    double rn = 1e9, rnf = 0;
    for (int f = 100; f <= 140; ++f) {
      double rr = sine_rms(h, SR, f, 0.3, 0.7, 2, &fin);
      if (rr < rn) { rn = rr; rnf = f; }
    }
    snprintf(msg, sizeof msg,
             "static comb (DEPTH 0): notch at %.0f Hz, %.1f dB deep "
             "(expect ~123 Hz: 4.65 ms plus the wet highpass phase)",
             rnf, db(dryref / rn));
    check(fin && db(dryref / rn) > 12.0 && rnf > 114 && rnf < 132, msg);
    ctrl[MIX] = 10.0f;
  }

  /* --- delay law: DEPTH 0 fixed, DEPTH 10 sweeps 3.7..6.2 ms -------- */
  {
    ctrl[DEPTH] = 0.0f;
    advance(h, (long)(0.5 * SR));
    double d0[4];
    double dmin = 1, dmax = 0;
    for (int k = 0; k < 4; ++k) {
      d0[k] = click_delay(h, SR);
      advance(h, (long)(0.4 * SR));
      dmin = fmin(dmin, d0[k]);
      dmax = fmax(dmax, d0[k]);
    }
    snprintf(msg, sizeof msg,
             "DEPTH 0: static delay %.2f..%.2f ms (expect 4.65 ms + filters)",
             dmin * 1e3, dmax * 1e3);
    check(dmin > 4.2e-3 && dmax < 5.7e-3 && (dmax - dmin) < 0.15e-3, msg);

    ctrl[DEPTH] = 10.0f;
    ctrl[RATE] = 0.0f;                 /* 0.3 Hz */
    advance(h, (long)(0.8 * SR));
    double lo = 1, hi = 0;
    for (int k = 0; k < 10; ++k) {
      double d = click_delay(h, SR);
      advance(h, (long)(0.30 * SR));   /* ~0.3 of the 3.33 s LFO cycle */
      lo = fmin(lo, d);
      hi = fmax(hi, d);
    }
    snprintf(msg, sizeof msg,
             "DEPTH 10: sweep %.2f..%.2f ms (expect 3.7..6.2 ms + filters)",
             lo * 1e3, hi * 1e3);
    check(lo > 3.4e-3 && hi < 7.0e-3 && (hi - lo) > 1.0e-3, msg);
  }

  /* --- RATE: LFO 0.3 Hz..3.5 Hz ----------------------------------- */
  {
    double seq[80];
    int nseq = 80;
    ctrl[DEPTH] = 10.0f;
    ctrl[RATE] = 10.0f;                /* 3.5 Hz */
    advance(h, (long)(0.5 * SR));
    for (int k = 0; k < nseq; ++k) {
      seq[k] = click_delay(h, SR);
      advance(h, (long)(0.025 * SR) - BLOCK);
    }
    int fast = 0;
    double prev = 0;
    for (int k = 1; k < nseq; ++k) {
      const double dv = seq[k] - seq[k - 1];
      if (fabs(dv) > 0.05e-3) {
        const double s = dv > 0 ? 1 : -1;
        if (prev != 0 && s != prev)
          ++fast;
        prev = s;
      }
    }
    ctrl[RATE] = 0.0f;                 /* 0.3 Hz */
    advance(h, (long)(0.3 * SR));
    for (int k = 0; k < nseq; ++k) {
      seq[k] = click_delay(h, SR);
      advance(h, (long)(0.025 * SR) - BLOCK);
    }
    int slow = 0;
    prev = 0;
    for (int k = 1; k < nseq; ++k) {
      const double dv = seq[k] - seq[k - 1];
      if (fabs(dv) > 0.05e-3) {
        const double s = dv > 0 ? 1 : -1;
        if (prev != 0 && s != prev)
          ++slow;
        prev = s;
      }
    }
    snprintf(msg, sizeof msg,
             "RATE: 3.5 Hz -> %d sweep reversals in 2 s, 0.3 Hz -> %d (expect ~14 vs <=3)",
             fast, slow);
    check(fast >= 8 && slow <= 3, msg);
  }

  /* --- MODEL: CE-2B (bass) cuts the wet below 40 Hz ---------------- */
  {
    ctrl[RATE] = 5.0f; ctrl[DEPTH] = 0.0f; ctrl[MIX] = 10.0f;
    ctrl[MODEL] = 0.0f;
    r1 = sine_rms(h, SR, 30, 0.3, 1.5, 8, &fin);
    ctrl[MODEL] = 1.0f;
    r2 = sine_rms(h, SR, 30, 0.3, 1.5, 8, &fin);
    snprintf(msg, sizeof msg,
             "MODEL: 30 Hz level bass %+.2f dB vs guitar (expect about -1.8 dB)",
             db(r2 / r1));
    check(fin && db(r2 / r1) < -0.8 && db(r2 / r1) > -3.5, msg);
    ctrl[MODEL] = 0.0f;
  }

  /* --- extremes: max everything, both models ------------------------ */
  for (int m = 0; m < 2; ++m) {
    ctrl[RATE] = 10.0f; ctrl[DEPTH] = 10.0f; ctrl[MIX] = 10.0f;
    ctrl[MODEL] = (float)m;
    r = sine_rms(h, SR, 437, 0.9, 2.0, 2, &fin);
    check(fin && r < 2.0, m ? "all max, bass model: finite, bounded"
                            : "all max, guitar model: finite, bounded");
  }
  defaults();

  /* --- bypass crossfade (lv2:enabled): click-free, back to dry ------ */
  {
    long ph = 0;
    double wetstep = 0, drystep = 0, fadestep = 0, md = 0, prev = 0;
    enabled = 1.0f;
    for (int b = 0; b < 48; ++b) {
      for (int i = 0; i < BLOCK; ++i, ++ph)
        in[i] = (float)(0.5 * sin(2.0 * M_PI * 220.0 * ph / SR));
      desc->run(h, BLOCK);
      for (int i = 0; i < BLOCK; ++i) {
        if (b == 47 && i > 0)
          wetstep = fmax(wetstep, fabs(out[i] - out[i - 1]));
        if (i > 0)
          drystep = fmax(drystep, fabs(in[i] - in[i - 1]));
      }
      prev = out[BLOCK - 1];
    }
    enabled = 0.0f;
    for (int b = 0; b < 4; ++b) {
      for (int i = 0; i < BLOCK; ++i, ++ph)
        in[i] = (float)(0.5 * sin(2.0 * M_PI * 220.0 * ph / SR));
      desc->run(h, BLOCK);
      for (int i = 0; i < BLOCK; ++i) {
        if (b < 2)
          fadestep = fmax(fadestep, fabs(out[i] - prev));
        prev = out[i];
      }
    }
    snprintf(msg, sizeof msg,
             "bypass fade: max step %.4f (wet %.4f, dry %.4f): no click",
             fadestep, wetstep, drystep);
    check(fadestep <= 1.2 * fmax(wetstep, drystep), msg);
    advance(h, BLOCK);
    maxdiff = 0;
    for (int i = 0; i < BLOCK; ++i)
      maxdiff = fmax(maxdiff, fabs((double)out[i] - in[i]));
    check(maxdiff < 1e-6, "bypassed again: output == input after the 10 ms fade");
    enabled = 1.0f;
    ctrl[MIX] = 10.0f;
    r = sine_rms(h, SR, 1000, 0.5, 0.5, 1, &fin);
    check(fin && r > 0.01, "re-enabled: effect back");
  }
  defaults();

  /* --- silence in, silence out ------------------------------------- */
  {
    /* the input highpass (7.2 Hz, tau = 22 ms) needs ~0.9 s to settle */
    memset(in, 0, sizeof(float) * BLOCK);
    for (int b = 0; b < 40; ++b)
      desc->run(h, BLOCK);
    double m = 0;
    for (int i = 0; i < BLOCK; ++i)
      m = fmax(m, fabs(out[i]));
    check(m < 1e-5, "silence in: silence out");
  }

  /* --- long run with random controls and noise ---------------------- */
  int stable = 1;
  srand(1);
  for (int b = 0; b < 4096 && stable; ++b) {
    if (b % 16 == 0) {
      ctrl[RATE] = rand() % 101 / 10.0f;
      ctrl[DEPTH] = rand() % 101 / 10.0f;
      ctrl[MIX] = rand() % 101 / 10.0f;
      ctrl[MODEL] = rand() % 2;
    }
    for (int i = 0; i < BLOCK; ++i)
      in[i] = (rand() % 2001 / 1000.0f - 1.0f) * 0.8f;
    desc->run(h, BLOCK);
    for (int i = 0; i < BLOCK; ++i)
      if (!isfinite(out[i]) || fabsf(out[i]) > 2.0f)
        stable = 0;
  }
  check(stable, "4096 blocks noise + random controls: finite, bounded");
  close_plugin(h);

  /* --- other sample rates ------------------------------------------- */
  const double rates[] = { 44100, 88200, 96000, 192000 };
  for (int i = 0; i < 4; ++i) {
    defaults();
    LV2_Handle h2 = open_plugin(rates[i]);
    r = h2 ? sine_rms(h2, rates[i], 1000, 0.5, 1.0, 1, &fin) : -1;
    snprintf(msg, sizeof msg, "@%.0f Hz: finite, sensible level (rms %.3f)",
             rates[i], r);
    check(h2 && fin && r > 0.01 && r < 2.0, msg);
    if (h2)
      close_plugin(h2);
  }

  dlclose(lib);
  printf("\n%s (%d failure%s)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED",
         failures, failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
