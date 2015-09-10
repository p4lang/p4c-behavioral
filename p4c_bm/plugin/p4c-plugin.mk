# command plugin makefile - includes others when enabled

ifdef PLUGIN_SAI
include ${P4FACTORY}/submodules/p4c-behavioral/p4c_bm/plugin/sai-plugin.mk
GLOBAL_CFLAGS += -DENABLE_PLUGIN_SAI
endif

ifdef PLUGIN_OPENFLOW
include ${P4FACTORY}/submodules/p4c-behavioral/p4c_bm/plugin/openflow-plugin.mk
endif
