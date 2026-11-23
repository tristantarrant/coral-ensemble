NAME = coralensemble
FILES_DSP = PluginCoralEnsemble.cpp
ifneq ($(NOUI),true)
FILES_UI  = UICoralEnsemble.cpp
UI_TYPE   = cairo
endif
include ../../dpf/Makefile.plugins.mk
BUILD_CXX_FLAGS += -Wno-unused-parameter -Wno-unused-variable
TARGETS ?= lv2_sep vst3 clap jack
all: $(TARGETS)
