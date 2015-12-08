
BM_TENJIN_TARGET := ${BM_TENJIN_OUTPUT_NEWEST}

BM_OF_PLUGIN_SRC_DIR := ${BM_BUILD_DIR}/plugin/of/src

BM_OF_TEMPLATES_C := $(wildcard ${THIS_DIR}/p4c_bm/plugin/of/src/*.c)
BM_OF_TENJIN_OUTPUT_C := $(addprefix ${BM_OF_PLUGIN_SRC_DIR}/, $(notdir ${BM_OF_TEMPLATES_C}))
BM_OF_OBJS_C := $(addsuffix .o, $(basename $(addprefix ${BM_OBJ_DIR}/, $(notdir ${BM_OF_TENJIN_OUTPUT_C}))))

GLOBAL_CFLAGS += -I $(SUBMODULE_P4OFAGENT)/inc
GLOBAL_CFLAGS += -I $(SUBMODULE_P4OFAGENT)/submodules/indigo/modules/indigo/module/inc
GLOBAL_CFLAGS += -I $(SUBMODULE_P4OFAGENT)/submodules/indigo/submodules/loxigen-artifacts/loci/inc
GLOBAL_CFLAGS += -I $(SUBMODULE_P4OFAGENT)/submodules/indigo/submodules/infra/modules/AIM/module/inc

${BM_OF_OBJS_C} : ${BM_OBJ_DIR}/%.o : ${BM_OF_PLUGIN_SRC_DIR}/%.c
	@echo "    Compiling : openflow::$(notdir $<)"
	$(VERBOSE)gcc -o $@ $(COVERAGE_FLAGS) $(DEBUG_FLAGS) $(GLOBAL_INCLUDES) $(GLOBAL_CFLAGS) -I $(PUBLIC_INC_PATH) -MD -c $<

# plugin target
PLUGIN_LIBS += plugin-openflow.a
$(LIB_DIR)/plugin-openflow.a: ${BM_OF_OBJS_C} ${BM_TENJIN_TARGET}
	ar -rc $@ $^

.NOTPARALLEL:
