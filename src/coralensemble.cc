// generated from file 'src/coralensemble.dsp' by dsp2cc:
// Code generated with Faust 2.85.9 (https://faust.grame.fr)


namespace coralensemble {

class Dsp: public PluginLV2 {
private:
	uint32_t fSampleRate;
	double fConst0;
	double fConst1;
	double fConst2;
	double fConst3;
	double fConst4;
	double fConst5;
	double fConst6;
	double fConst7;
	double fConst8;
	double fConst9;
	double fConst10;
	double fConst11;
	double fConst12;
	double fConst13;
	double fConst14;
	FAUSTFLOAT fEntry0;
	FAUSTFLOAT	*fEntry0_;
	double fConst15;
	double fConst16;
	double fConst17;
	double fConst18;
	double fConst19;
	double fConst20;
	double fVec0[2];
	double fConst21;
	double fConst22;
	double fConst23;
	double fRec8[2];
	double fConst24;
	double fConst25;
	double fConst26;
	double fRec7[2];
	double fRec6[3];
	double fConst27;
	double fConst28;
	double fConst29;
	double fVec1[2];
	double fConst30;
	double fConst31;
	double fConst32;
	double fConst33;
	double fConst34;
	double fRec5[2];
	double fRec4[4];
	double fConst35;
	double fConst36;
	double fConst37;
	double fConst38;
	double fConst39;
	int IOTA0;
	double fVec2[2048];
	int iConst40;
	FAUSTFLOAT fHslider0;
	FAUSTFLOAT	*fHslider0_;
	double fRec11[2];
	double fConst41;
	double fRec10[2];
	double fVec3[2];
	double fConst42;
	double fConst43;
	double fConst44;
	double fRec9[2];
	FAUSTFLOAT fHslider1;
	FAUSTFLOAT	*fHslider1_;
	double fRec12[2];
	double fConst45;
	double fVec4[2];
	double fRec3[2];
	double fRec2[4];
	FAUSTFLOAT fHslider2;
	FAUSTFLOAT	*fHslider2_;
	double fRec13[2];
	double fConst46;
	double fRec1[3];
	double fConst47;
	double fConst48;
	double fConst49;
	double fVec5[2];
	double fConst50;
	double fConst51;
	double fConst52;
	double fConst53;
	double fConst54;
	double fRec0[2];

	void connect(uint32_t port,void* data);
	void clear_state_f();
	void init(uint32_t sample_rate);
	void compute(int count, FAUSTFLOAT *input0, FAUSTFLOAT *output0);

	static void clear_state_f_static(PluginLV2*);
	static void init_static(uint32_t sample_rate, PluginLV2*);
	static void compute_static(int count, FAUSTFLOAT *input0, FAUSTFLOAT *output0, PluginLV2*);
	static void del_instance(PluginLV2 *p);
	static void connect_static(uint32_t port,void* data, PluginLV2 *p);
public:
	Dsp();
	~Dsp();
};



Dsp::Dsp()
	: PluginLV2() {
	version = PLUGINLV2_VERSION;
	id = "coralensemble";
	name = N_("Coral Ensemble");
	mono_audio = compute_static;
	stereo_audio = 0;
	set_samplerate = init_static;
	activate_plugin = 0;
	connect_ports = connect_static;
	clear_state = clear_state_f_static;
	delete_instance = del_instance;
}

Dsp::~Dsp() {
}

inline void Dsp::clear_state_f()
{
	for (int l0 = 0; l0 < 2; l0 = l0 + 1) fVec0[l0] = 0.0;
	for (int l1 = 0; l1 < 2; l1 = l1 + 1) fRec8[l1] = 0.0;
	for (int l2 = 0; l2 < 2; l2 = l2 + 1) fRec7[l2] = 0.0;
	for (int l3 = 0; l3 < 3; l3 = l3 + 1) fRec6[l3] = 0.0;
	for (int l4 = 0; l4 < 2; l4 = l4 + 1) fVec1[l4] = 0.0;
	for (int l5 = 0; l5 < 2; l5 = l5 + 1) fRec5[l5] = 0.0;
	for (int l6 = 0; l6 < 4; l6 = l6 + 1) fRec4[l6] = 0.0;
	for (int l7 = 0; l7 < 2048; l7 = l7 + 1) fVec2[l7] = 0.0;
	for (int l8 = 0; l8 < 2; l8 = l8 + 1) fRec11[l8] = 0.0;
	for (int l9 = 0; l9 < 2; l9 = l9 + 1) fRec10[l9] = 0.0;
	for (int l10 = 0; l10 < 2; l10 = l10 + 1) fVec3[l10] = 0.0;
	for (int l11 = 0; l11 < 2; l11 = l11 + 1) fRec9[l11] = 0.0;
	for (int l12 = 0; l12 < 2; l12 = l12 + 1) fRec12[l12] = 0.0;
	for (int l13 = 0; l13 < 2; l13 = l13 + 1) fVec4[l13] = 0.0;
	for (int l14 = 0; l14 < 2; l14 = l14 + 1) fRec3[l14] = 0.0;
	for (int l15 = 0; l15 < 4; l15 = l15 + 1) fRec2[l15] = 0.0;
	for (int l16 = 0; l16 < 2; l16 = l16 + 1) fRec13[l16] = 0.0;
	for (int l17 = 0; l17 < 3; l17 = l17 + 1) fRec1[l17] = 0.0;
	for (int l18 = 0; l18 < 2; l18 = l18 + 1) fVec5[l18] = 0.0;
	for (int l19 = 0; l19 < 2; l19 = l19 + 1) fRec0[l19] = 0.0;
}

void Dsp::clear_state_f_static(PluginLV2 *p)
{
	static_cast<Dsp*>(p)->clear_state_f();
}

inline void Dsp::init(uint32_t sample_rate)
{
	fSampleRate = sample_rate;
	fConst0 = std::min<double>(1.92e+05, std::max<double>(1.0, static_cast<double>(fSampleRate)));
	fConst1 = 1.88e+09 * fConst0;
	fConst2 = fConst0 * (fConst1 + -1153823529411764.8) + 1.4705882352941176e+18;
	fConst3 = mydsp_faustpower2_f(fConst0);
	fConst4 = 2.9411764705882353e+18 - 3.76e+09 * fConst3;
	fConst5 = fConst0 * (fConst1 + 1153823529411764.8) + 1.4705882352941176e+18;
	fConst6 = 1.0 / fConst5;
	fConst7 = 508728.0 * fConst3;
	fConst8 = 7.574e+10 * fConst0;
	fConst9 = fConst0 * (fConst8 + -1.291e+15 - fConst7) + 5e+18;
	fConst10 = 1526184.0 * fConst3;
	fConst11 = fConst0 * (fConst10 + (-1.291e+15 - fConst8)) + 1.5e+19;
	fConst12 = fConst0 * (1.291e+15 - fConst8 - fConst10) + 1.5e+19;
	fConst13 = fConst0 * (fConst8 + 1.291e+15 + fConst7) + 5e+18;
	fConst14 = 1.0 / fConst13;
	fConst15 = 1.5151515151515152e-06 / fConst0;
	fConst16 = 8.836e+13 * fConst0;
	fConst17 = fConst0 * (fConst16 + -1.0049705882352941e+19) + 6.911764705882353e+22;
	fConst18 = 1.3823529411764706e+23 - 1.7672e+14 * fConst3;
	fConst19 = fConst0 * (fConst16 + 1.0049705882352941e+19) + 6.911764705882353e+22;
	fConst20 = 1.0 / fConst19;
	fConst21 = 1.0 / std::tan(22.58661968649772 / fConst0);
	fConst22 = 1.0 - fConst21;
	fConst23 = 1.0 / (fConst21 + 1.0);
	fConst24 = 1.0 / std::tan(22.634676324128563 / fConst0);
	fConst25 = 1.0 - fConst24;
	fConst26 = 1.0 / (fConst24 + 1.0);
	fConst27 = 5.358e+19 * fConst0;
	fConst28 = fConst27 + -6.911764705882353e+22;
	fConst29 = -6.911764705882353e+22 - fConst27;
	fConst30 = std::tan(151.51515151515153 / fConst0);
	fConst31 = 1.0 / (fConst30 * fConst19);
	fConst32 = 1.0 / fConst30;
	fConst33 = 1.0 - fConst32;
	fConst34 = 1.0 / (fConst32 + 1.0);
	fConst35 = 8.2e+14 * fConst0;
	fConst36 = 5e+18 - fConst35;
	fConst37 = 1.5e+19 - fConst35;
	fConst38 = fConst35 + 1.5e+19;
	fConst39 = fConst35 + 5e+18;
	iConst40 = static_cast<int>(0.008 * fConst0) + 8;
	fConst41 = 1.0 / fConst0;
	fConst42 = 1.0 / std::tan(226.1946710584651 / fConst0);
	fConst43 = 1.0 - fConst42;
	fConst44 = 1.0 / (fConst42 + 1.0);
	fConst45 = 0.004654545454545455 * fConst0;
	fConst46 = 0.1 / fConst13;
	fConst47 = 2e+14 * fConst0;
	fConst48 = fConst47 + -1.4705882352941176e+18;
	fConst49 = -1.4705882352941176e+18 - fConst47;
	fConst50 = std::tan(5.000000000000001 / fConst0);
	fConst51 = 1.0 / (fConst50 * fConst5);
	fConst52 = 1.0 / fConst50;
	fConst53 = 1.0 - fConst52;
	fConst54 = 1.0 / (fConst52 + 1.0);
	IOTA0 = 0;
	clear_state_f();
}

void Dsp::init_static(uint32_t sample_rate, PluginLV2 *p)
{
	static_cast<Dsp*>(p)->init(sample_rate);
}

void always_inline Dsp::compute(int count, FAUSTFLOAT *input0, FAUSTFLOAT *output0)
{
#define fEntry0 (*fEntry0_)
#define fHslider0 (*fHslider0_)
#define fHslider1 (*fHslider1_)
#define fHslider2 (*fHslider2_)
	double fSlow0 = std::tan(fConst15 / ((static_cast<int>(static_cast<double>(fEntry0))) ? 1.2e-08 : 3.3e-08));
	double fSlow1 = 1.0 / fSlow0;
	double fSlow2 = fSlow1 + 1.0;
	double fSlow3 = 0.0010000000000000009 * static_cast<double>(fHslider0);
	double fSlow4 = 0.0010000000000000009 * static_cast<double>(fHslider1);
	double fSlow5 = 1.0 - fSlow1;
	double fSlow6 = 0.0010000000000000009 * static_cast<double>(fHslider2);
	for (int i0 = 0; i0 < count; i0 = i0 + 1) {
		double fTemp0 = static_cast<double>(input0[i0]);
		fVec0[0] = fTemp0;
		fRec8[0] = -(fConst23 * (fConst22 * fRec8[1] - fConst21 * (fTemp0 - fVec0[1])));
		fRec7[0] = -(fConst26 * (fConst25 * fRec7[1] - fConst24 * (fRec8[0] - fRec8[1])));
		fRec6[0] = fRec7[0] - fConst20 * (fConst18 * fRec6[1] + fConst17 * fRec6[2]);
		double fTemp1 = fConst29 * fRec6[0] - 1.3823529411764706e+23 * fRec6[1] + fConst28 * fRec6[2];
		fVec1[0] = fTemp1;
		fRec5[0] = -(fConst34 * (fConst33 * fRec5[1] - fConst31 * (fTemp1 - fVec1[1])));
		fRec4[0] = fRec5[0] - fConst14 * (fConst12 * fRec4[1] + fConst11 * fRec4[2] + fConst9 * fRec4[3]);
		double fTemp2 = fConst39 * fRec4[0] + fConst38 * fRec4[1] + fConst37 * fRec4[2] + fConst36 * fRec4[3];
		double fTemp3 = fTemp2 / std::sqrt(std::sqrt(mydsp_faustpower4_f(0.4 * std::fabs(fConst14 * fTemp2)) + 1.0));
		fVec2[IOTA0 & 2047] = fTemp3;
		fRec11[0] = fSlow3 + 0.999 * fRec11[1];
		double fTemp4 = fRec10[1] + fConst41 * (0.32 * fRec11[0] + 0.3);
		fRec10[0] = fTemp4 - std::floor(fTemp4);
		double fTemp5 = 2.0 * (1.0 - std::fabs(2.0 * fRec10[0] + -1.0)) + -1.0;
		fVec3[0] = fTemp5;
		fRec9[0] = -(fConst44 * (fConst43 * fRec9[1] - (fTemp5 + fVec3[1])));
		fRec12[0] = fSlow4 + 0.999 * fRec12[1];
		double fTemp6 = fConst45 / (1.0 - 0.0246 * fRec12[0] * fRec9[0]);
		double fTemp7 = fTemp6 + -1.499995;
		int iTemp8 = static_cast<int>(fTemp7);
		double fTemp9 = std::floor(fTemp7);
		double fTemp10 = fTemp6 + (-3.0 - fTemp9);
		double fTemp11 = fTemp6 + (-2.0 - fTemp9);
		double fTemp12 = fTemp6 + (-1.0 - fTemp9);
		double fTemp13 = fTemp6 - fTemp9;
		double fTemp14 = fTemp13 * fTemp12;
		double fTemp15 = fTemp14 * fTemp11;
		double fTemp16 = (fTemp6 + (-4.0 - fTemp9)) * (fTemp10 * (fTemp11 * (0.041666666666666664 * fVec2[(IOTA0 - std::min<int>(iConst40, std::max<int>(0, iTemp8))) & 2047] * fTemp12 - 0.16666666666666666 * fTemp13 * fVec2[(IOTA0 - std::min<int>(iConst40, std::max<int>(0, iTemp8 + 1))) & 2047]) + 0.25 * fTemp14 * fVec2[(IOTA0 - std::min<int>(iConst40, std::max<int>(0, iTemp8 + 2))) & 2047]) - 0.16666666666666666 * fTemp15 * fVec2[(IOTA0 - std::min<int>(iConst40, std::max<int>(0, iTemp8 + 3))) & 2047]) + 0.041666666666666664 * fTemp15 * fTemp10 * fVec2[(IOTA0 - std::min<int>(iConst40, std::max<int>(0, iTemp8 + 4))) & 2047];
		fVec4[0] = fTemp16;
		fRec3[0] = -((fRec3[1] * fSlow5 - fConst14 * ((fTemp16 - fVec4[1]) / fSlow0)) / fSlow2);
		fRec2[0] = fRec3[0] - fConst14 * (fConst12 * fRec2[1] + fConst11 * fRec2[2] + fConst9 * fRec2[3]);
		fRec13[0] = fSlow6 + 0.999 * fRec13[1];
		fRec1[0] = fConst20 * fTemp1 + fConst46 * fRec13[0] * (fConst39 * fRec2[0] + fConst38 * fRec2[1] + fConst37 * fRec2[2] + fConst36 * fRec2[3]) - fConst6 * (fConst4 * fRec1[1] + fConst2 * fRec1[2]);
		double fTemp17 = fConst49 * fRec1[0] - 2.9411764705882353e+18 * fRec1[1] + fConst48 * fRec1[2];
		fVec5[0] = fTemp17;
		fRec0[0] = -(fConst54 * (fConst53 * fRec0[1] - fConst51 * (fTemp17 - fVec5[1])));
		output0[i0] = static_cast<FAUSTFLOAT>(fRec0[0]);
		fVec0[1] = fVec0[0];
		fRec8[1] = fRec8[0];
		fRec7[1] = fRec7[0];
		fRec6[2] = fRec6[1];
		fRec6[1] = fRec6[0];
		fVec1[1] = fVec1[0];
		fRec5[1] = fRec5[0];
		for (int j0 = 3; j0 > 0; j0 = j0 - 1) {
			fRec4[j0] = fRec4[j0 - 1];
		}
		IOTA0 = IOTA0 + 1;
		fRec11[1] = fRec11[0];
		fRec10[1] = fRec10[0];
		fVec3[1] = fVec3[0];
		fRec9[1] = fRec9[0];
		fRec12[1] = fRec12[0];
		fVec4[1] = fVec4[0];
		fRec3[1] = fRec3[0];
		for (int j1 = 3; j1 > 0; j1 = j1 - 1) {
			fRec2[j1] = fRec2[j1 - 1];
		}
		fRec13[1] = fRec13[0];
		fRec1[2] = fRec1[1];
		fRec1[1] = fRec1[0];
		fVec5[1] = fVec5[0];
		fRec0[1] = fRec0[0];
	}
#undef fEntry0
#undef fHslider0
#undef fHslider1
#undef fHslider2
}

void __rt_func Dsp::compute_static(int count, FAUSTFLOAT *input0, FAUSTFLOAT *output0, PluginLV2 *p)
{
	static_cast<Dsp*>(p)->compute(count, input0, output0);
}


void Dsp::connect(uint32_t port,void* data)
{
	switch ((PortIndex)port)
	{
	case DEPTH: 
		fHslider1_ = (float*)data; // , 5.0, 0.0, 1e+01, 0.1 
		break;
	case MIX: 
		fHslider2_ = (float*)data; // , 1e+01, 0.0, 1e+01, 0.1 
		break;
	case RATE: 
		fHslider0_ = (float*)data; // , 5.0, 0.0, 1e+01, 0.1 
		break;
	case MODEL: 
		fEntry0_ = (float*)data; // , 0.0, 0.0, 1.0, 1.0 
		break;
	default:
		break;
	}
}

void Dsp::connect_static(uint32_t port,void* data, PluginLV2 *p)
{
	static_cast<Dsp*>(p)->connect(port, data);
}


PluginLV2 *plugin() {
	return new Dsp();
}

void Dsp::del_instance(PluginLV2 *p)
{
	delete static_cast<Dsp*>(p);
}

/*
typedef enum
{
   DEPTH, 
   MIX, 
   MODEL, 
   RATE, 
} PortIndex;
*/

} // end namespace coralensemble
