
BM_TENJIN_TARGET := ${BM_TENJIN_OUTPUT_NEWEST}

BM_BUILD_PLUGIN_SRC_DIR := ${BM_BUILD_DIR}/plugin/sai/src
BM_BUILD_PLUGIN_THRIFT_DIR := ${BM_BUILD_DIR}/plugin/sai/thrift
${BM_BUILD_PLUGIN_SRC_DIR}/p4_sai_rpc_types.h : ${BM_BUILD_PLUGIN_THRIFT_DIR}/p4_sai_rpc.thrift # ${BM_TENJIN_TARGET}
	thrift --gen cpp --out ${BM_BUILD_PLUGIN_SRC_DIR} $<

${BM_PLUGIN_THRIFT_OUTPUT_CPP} : ${BM_BUILD_PLUGIN_SRC_DIR}/p4_sai_rpc_types.h

THRIFT_INPUT_FILES := ${BM_BUILD_PLUGIN_THRIFT_DIR}/p4_sai_rpc.thrift
THRIFT_DEP_FILES := ${BM_BUILD_PLUGIN_THRIFT_DIR}/p4_sai_rpc.thrift
THRIFT_SERVICE_NAMES := p4_sai_rpc
include ${MAKEFILES_DIR}/thrift-py.mk

${BM_BUILD_PLUGIN_THRIFT_DIR}/p4_sai_rpc.thrift: $(BM_TENJIN_OUTPUT_OLDEST) 

BM_TEMPLATES_SAI_C := $(addprefix ${BM_BUILD_PLUGIN_SRC_DIR}/, sai.c sai_templ.c sai_static.c)
BM_TEMPLATES_SAI := ${BM_TEMPLATES_SAI_C} ${BM_TEMPLATES_SAI_CPP}
BM_TENJIN_OUTPUT_SAI_C := $(addprefix ${BM_BUILD_PLUGIN_SRC_DIR}/, $(notdir ${BM_TEMPLATES_SAI_C}))
BM_SAI_OBJS_C := $(addsuffix .o, $(basename $(addprefix ${BM_OBJ_DIR}/, $(notdir ${BM_TENJIN_OUTPUT_SAI_C}))))
${BM_SAI_OBJS_C} : ${BM_OBJ_DIR}/%.o : ${BM_BUILD_PLUGIN_SRC_DIR}/%.c ${BM_TEMPLATES_SAI}
	@echo "    Compiling : sai::$(notdir $<)"
	$(VERBOSE)gcc -o $@ $(COVERAGE_FLAGS) $(DEBUG_FLAGS) $(GLOBAL_INCLUDES) $(GLOBAL_CFLAGS) -I $(PUBLIC_INC_PATH) -MD -c $<


BM_TEMPLATES_SAI_CPP := $(addprefix ${BM_BUILD_PLUGIN_SRC_DIR}/, p4_sai_rpc_constants.cpp p4_sai_rpc_types.cpp p4_sai_rpc_server.cpp sai_p4_sai.cpp)
BM_TENJIN_OUTPUT_SAI_CPP := $(addprefix ${BM_BUILD_PLUGIN_SRC_DIR}/, $(notdir ${BM_TEMPLATES_SAI_CPP}))
BM_SAI_OBJS_CPP := $(addsuffix .o, $(basename $(addprefix ${BM_OBJ_DIR}/, $(notdir ${BM_TENJIN_OUTPUT_SAI_CPP}))))

${BM_SAI_OBJS_CPP} : ${BM_OBJ_DIR}/%.o : ${BM_BUILD_PLUGIN_SRC_DIR}/%.cpp ${BM_TEMPLATES_SAI}
	@echo "    Compiling : sai::$(notdir $<)"
	$(VERBOSE)g++ -o $@ $(COVERAGE_FLAGS) $(DEBUG_FLAGS) $(GLOBAL_INCLUDES) -I ${PUBLIC_INC_PATH} -MD -c $<

# plugin target
$(LIB_DIR)/plugin-sai.a: ${BM_PLUGIN_THRIFT_OUTPUT_CPP} ${BM_BUILD_PLUGIN_SRC_DIR}/p4_sai_rpc_types.h ${BM_SAI_OBJS_C} ${BM_SAI_OBJS_CPP} 
	ar -rc $@ $^

.NOTPARALLEL:
