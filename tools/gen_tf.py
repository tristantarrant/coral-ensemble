#!/usr/bin/env python3
"""
Generate src/coralensemble_tf.lib: the linear stages of the BOSS CE-2
Chorus Ensemble, solved by symbolic nodal analysis.

Circuit sources:
  * Boss CE-2 service schematic (Feb 1987 service notes, page 1);
  * Aion FX "Azure Analog Chorus" trace of a real CE-2 (documentation
    rev 1.1.3, 2025), used for the complete component set;
  * ElectroSmash "Boss CE-2 Analysis" (stage-by-stage description).

Stages:
  pre      IC1B pre-emphasis. Inverting shelving: R4 47k in the input leg,
           C3 6n8 + R5 10k in series in parallel with it, R6 47k || C4 100p
           in the feedback. Gain -1 below 410 Hz, the treble boost above
           2.34 kHz (limited by C4 above ~10 kHz).
  deemph   IC1A de-emphasis and dry/wet summing amp. R21 47k (dry) and
           R22 47k (wet) into the virtual ground, feedback
           R24 47k || (R23 10k + C15 6n8) || C16 100p. Gain per input: -1
           below 410 Hz and X1/
  sk       3rd-order Sallen-Key low-pass with an emitter follower (Q2 before
           the BBD, Q3 after it; the two filters share the same value set:
           10k/10k/10k, 3n3, 8n2, 470p): R14 in->A, C9 A-gnd, R15 A->X,
           C10 X->out (bootstrap), R16 X->base, C11 base-gnd, out = base.

The two 1st-order highpasses (C8 33n/100k = 48 Hz into the BBD, C14
33n/330k = 14.6 Hz, 12n = 40 Hz for the CE-2B after it) and the input/output
highpasses are simple RC networks and are written directly as
fi.highpass(1, fc) in the DSP.

Usage: tools/gen_tf.py > src/coralensemble_tf.lib
       tools/gen_tf.py --verify        (responses and pole radii)
"""

import sys
import sympy as sp

s = sp.Symbol("s")

k = 1e3
n = 1e-9
p = 1e-12

# Boss CE-2 component values (service schematic / Aion FX trace)
PRE = dict(Rin=47 * k, C3=6.8 * n, Rs=10 * k, Rf=47 * k, Cf=100 * p)
DE = dict(R1=47 * k, R2=47 * k, Rf=47 * k, Rs=10 * k, Cf1=6.8 * n, Cf2=100 * p)
SK = dict(R1=10 * k, C1=3.3 * n, R2=10 * k, C2=8.2 * n, R3=10 * k, C3=470 * p)


def par(a, b):
    return sp.simplify(a * b / (a + b))


def poly_asc(expr):
    num, den = sp.fraction(sp.cancel(sp.together(expr)))
    B = list(reversed(sp.Poly(sp.expand(num), s).all_coeffs()))
    A = list(reversed(sp.Poly(sp.expand(den), s).all_coeffs()))
    return B, A


def stage_pre(c):
    Zin = par(c["Rin"], c["Rs"] + 1 / (s * c["C3"]))
    Zf = par(c["Rf"], 1 / (s * c["Cf"]))
    return poly_asc(-Zf / Zin)


def stage_deemph(c):
    Zf = par(par(c["Rf"], c["Rs"] + 1 / (s * c["Cf1"])), 1 / (s * c["Cf2"]))
    return poly_asc(-Zf / c["R1"])


def stage_sk(c):
    """3rd-order Sallen-Key with an emitter follower. Node 'in' is the
    source, A/C9 the first RC, X the R15/R16 junction with the bootstrap
    cap C10 to the follower output O, C11 from the base (O) to ground."""
    el = [
        ("R", "in", "A", c["R1"]),
        ("C", "A", "gnd", c["C1"]),
        ("R", "A", "X", c["R2"]),
        ("C", "X", "O", c["C2"]),
        ("R", "X", "O", c["R3"]),
        ("C", "O", "gnd", c["C3"]),
    ]
    return solve(el, ["A", "X", "O"], "O")


def solve(elements, unknown, out):
    V = {nd: sp.Symbol("V_" + nd) for nd in unknown}
    V["gnd"] = 0
    V["in"] = 1
    eqs = []
    for nd in unknown:
        e = 0
        for kind, a, b, val in elements:
            Y = (1 / sp.nsimplify(val)) if kind == "R" else s * sp.nsimplify(val)
            if a == nd:
                e += Y * (V[a] - V[b])
            elif b == nd:
                e += Y * (V[b] - V[a])
        eqs.append(e)
    sol = sp.solve(eqs, [V[nd] for nd in unknown], dict=True)[0]
    return poly_asc(sol[V[out]])


def responses(B, A, freqs):
    Hs = sum(b * s**i for i, b in enumerate(B)) / sum(a * s**i for i, a in enumerate(A))
    out = []
    for f in freqs:
        val = complex(sp.N(Hs.subs(s, 2j * sp.pi * f)))
        out.append((f, abs(val), sp.N(sp.arg(val) * 180 / sp.pi, 4)))
    return out


def bilinear_poles(A, sr):
    """Roots of the digital denominator after the bilinear transform."""
    import numpy as np
    N = len(A) - 1
    c = 2.0 * sr
    # s = c (z - 1)/(z + 1); multiply by (z + 1)^N, ascending powers of z
    z = sp.Symbol("z")
    coef = []
    for j in range(N + 1):
        e = 0
        for i, a in enumerate(A):
            term = a * c**i * (z - 1) ** i * (z + 1) ** (N - i)
            e += float(sp.Poly(sp.expand(term), z).coeff_monomial(z**j))
        coef.append(e)
    return list(np.roots(list(reversed(coef))))


def fexpr(e):
    return sp.ccode(sp.N(e, 17))


def emit(name, doc, B, A):
    N = max(len(B), len(A)) - 1
    B = B + [sp.Integer(0)] * (N + 1 - len(B))
    A = A + [sp.Integer(0)] * (N + 1 - len(A))
    out = ["// " + doc]
    out.append("%s = fi.iir((%s), (%s)) with {" % (
        name,
        ", ".join("b%d/a0" % j for j in range(N + 1)),
        ", ".join("a%d/a0" % j for j in range(1, N + 1))))
    out.append("    c = 2.0*ma.SR;")
    for j in range(N + 1):
        out.append("    B%d = %s;" % (j, fexpr(B[j])))
    for j in range(N + 1):
        out.append("    A%d = %s;" % (j, fexpr(A[j])))
    for j in range(1, N + 1):
        out.append("    c%d = %s;" % (j, "*".join(["c"] * j)))
    # ascending powers of s: b0 + b1*s + b2*s^2 ...
    w = sp.Symbol("w")  # w = z^-1
    for j in range(N + 1):
        tb = []
        ta = []
        for kk in range(N + 1):
            # binomial coefficients of (1 - z^-1)^kk (1 + z^-1)^(N-kk)
            mjk = sp.expand((1 - w) ** kk * (1 + w) ** (N - kk))
            coef = int(sp.Poly(mjk, w).coeff_monomial(w**j))
            if coef == 0:
                continue
            ck = "" if kk == 0 else "*c%d" % kk
            tb.append("(%d)*B%d%s" % (coef, kk, ck))
            ta.append("(%d)*A%d%s" % (coef, kk, ck))
        out.append("    b%d = %s;" % (j, " + ".join(tb)))
        out.append("    a%d = %s;" % (j, " + ".join(ta)))
    out.append("};")
    out.append("")
    return "\n".join(out)


def main():
    parts = []
    parts.append("""// GENERATED by tools/gen_tf.py -- do not edit by hand.
//
// Linear stages of the BOSS CE-2 Chorus Ensemble, solved by nodal analysis
// from the Boss service schematic and the Aion FX "Azure" trace of a real
// CE-2. Analog H(s) coefficients are discretized at run time with the
// bilinear transform (c = 2*SR).

fi = library("filters.lib");
ma = library("maths.lib");
""")
    pre_B, pre_A = stage_pre(PRE)
    de_B, de_A = stage_deemph(DE)
    sk_B, sk_A = stage_sk(SK)

    parts.append(emit("pre",
        "IC1B pre-emphasis: inverting shelving (R4 47k, C3 6n8 + R5 10k, R6 47k || C4 100p)",
        pre_B, pre_A))
    parts.append(emit("deemph",
        "IC1A de-emphasis + dry/wet summing (R21/R22 47k, R24 47k || (R23 10k + C15 6n8) || C16 100p)",
        de_B, de_A))
    parts.append(emit("sk",
        "3rd-order Sallen-Key LPF (10k/10k/10k, 3n3, 8n2, 470p), Q2/Q3 follower: before and after the BBD",
        sk_B, sk_A))
    sys.stdout.write("\n".join(parts))


def digital_ba(B, A, sr):
    """Bilinear b/a from the analog polynomials, same expansion as emit()."""
    N = max(len(B), len(A)) - 1
    B = list(B) + [sp.Integer(0)] * (N + 1 - len(B))
    A = list(A) + [sp.Integer(0)] * (N + 1 - len(A))
    c = 2.0 * sr
    w = sp.Symbol("w")
    bb, aa = [], []
    for j in range(N + 1):
        sb = 0.0
        sa = 0.0
        for i in range(N + 1):
            mjk = sp.expand((1 - w) ** i * (1 + w) ** (N - i))
            coef = int(sp.Poly(mjk, w).coeff_monomial(w**j))
            sb += float(B[i]) * c**i * coef
            sa += float(A[i]) * c**i * coef
        bb.append(sb)
        aa.append(sa)
    return bb, aa


def digital_check(B, A, sr):
    """Compare the discretized transfer with the analog one (bilinear
    places analog f_a at digital f, so compare at the prewarped point)."""
    import cmath
    import math
    bb, aa = digital_ba(B, A, sr)

    def resp(f):
        z1 = cmath.exp(-2j * math.pi * f / sr)
        return sum(b * z1**j for j, b in enumerate(bb)) / sum(a * z1**j for j, a in enumerate(aa))

    out = []
    for f in (100, 1000, 5000):
        f_a = (2 * sr) * math.tan(math.pi * f / sr) / (2 * math.pi)
        an = [m for ff, m, _ in responses(B, A, [f_a])][0]
        d = 20 * math.log(abs(resp(f)), 10)
        a = 20 * math.log(float(an), 10)
        out.append("%5d Hz: dig %6.2f dB, ana %6.2f dB" % (f, d, a))
    return "; ".join(out)


def verify():
    pre_B, pre_A = stage_pre(PRE)
    de_B, de_A = stage_deemph(DE)
    sk_B, sk_A = stage_sk(SK)
    freqs = [20, 48, 100, 410, 1000, 2340, 5000, 6600, 10000, 15000, 20000]
    for name, (B, A) in (("pre", (pre_B, pre_A)), ("deemph", (de_B, de_A)), ("sk", (sk_B, sk_A))):
        print("== %s (order %d)" % (name, len(A) - 1))
        for f, mag, ph in responses(B, A, freqs):
            print("  %7.0f Hz  %7.2f dB  %8.1f deg" % (f, 20 * sp.log(mag, 10), ph))
    # dry path = pre * deemph must be flat (the schematic's design intent)
    print("== dry path pre*deemph")
    for f, mag, ph in responses(pre_B, pre_A, freqs):
        mag2 = [m for ff, m, _ in responses(de_B, de_A, [f])][0]
        print("  %7.0f Hz  %7.2f dB  %8.1f deg" % (
            f, 20 * sp.log(mag * mag2, 10), ph + [q for ff, _, q in responses(de_B, de_A, [f])][0]))
    for sr in (44100, 48000, 96000, 192000):
        radii = []
        for name, (B, A) in (("pre", (pre_B, pre_A)), ("deemph", (de_B, de_A)), ("sk", (sk_B, sk_A))):
            rr = [abs(z) for z in bilinear_poles(A, sr)]
            radii.append("%s max|z|=%.4f" % (name, max(rr)))
        print("SR %6d: %s" % (sr, "; ".join(radii)))
    print("== digital vs analog (bilinear), SR 48000")
    for name, (B, A) in (("pre", (pre_B, pre_A)), ("deemph", (de_B, de_A)), ("sk", (sk_B, sk_A))):
        print("  %-7s %s" % (name, digital_check(B, A, 48000)))


if __name__ == "__main__":
    if "--verify" in sys.argv:
        verify()
    else:
        main()
