THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

ifndef P4FACTORY
  $(error P4FACTORY not defined)
endif
MAKEFILES_DIR := $(P4FACTORY)/makefiles

ifndef SUBMODULE_P4C_BEHAVIORAL
  $(error SUBMODULE_P4C_BEHAVIORAL not defined)
endif

ifndef P4_NAME
  $(error P4_NAME not defined)
endif

ifndef P4_PREFIX
  P4_PREFIX := ${P4_NAME}
endif

# Behavioral compiler; default is to use the one in local submodule
BM_SHELL := ${SUBMODULE_P4C_BEHAVIORAL}/p4c_bm/shell.py

BM_BUILD_DIR := ${BUILD_DIR}/bm
BM_OBJ_DIR := ${BM_BUILD_DIR}/obj
BM_BUILD_SRC_DIR := ${BM_BUILD_DIR}/src
BM_BUILD_THRIFT_DIR := ${BM_BUILD_DIR}/thrift

MAKE_DIR := ${BM_BUILD_DIR}
include $(MAKEFILES_DIR)/makedir.mk

MAKE_DIR := ${BM_OBJ_DIR}
include $(MAKEFILES_DIR)/makedir.mk

MAKE_DIR := $(BM_BUILD_SRC_DIR)
include $(MAKEFILES_DIR)/makedir.mk

MAKE_DIR := $(PUBLIC_INC_PATH)/p4_sim
include $(MAKEFILES_DIR)/makedir.mk

BM_PARAMS := --gen-dir=${BM_BUILD_DIR}/ --thrift --public-inc-path=$(PUBLIC_INC_PATH)/p4_sim

BM_PARAMS += --p4-prefix=${P4_PREFIX}

ifdef BM_P4_META_CONFIG
  BM_PARAMS += --meta-config=${BM_P4_META_CONFIG}
endif

BM_TEMPLATES_C := $(wildcard ${THIS_DIR}/p4c_bm/templates/src/*.c)
BM_TEMPLATES_H := $(wildcard ${THIS_DIR}/p4c_bm/templates/src/*.h)
BM_TEMPLATES_CPP := $(wildcard ${THIS_DIR}/p4c_bm/templates/src/*.cpp)
BM_TEMPLATES_PUBLIC_HEADERS := $(wildcard ${THIS_DIR}/p4c_bm/templates/inc/*.h)
BM_TEMPLATES := ${BM_TEMPLATES_C} ${BM_TEMPLATES_H} ${BM_TEMPLATES_CPP} ${BM_TEMPLATES_PUBLIC_HEADERS}

BM_TENJIN_OUTPUT_C := $(addprefix ${BM_BUILD_SRC_DIR}/, $(notdir ${BM_TEMPLATES_C}))
BM_TENJIN_OUTPUT_H := $(addprefix ${BM_BUILD_DIR}/src/, $(notdir ${BM_TEMPLATES_H}))
BM_TENJIN_OUTPUT_CPP += $(addprefix ${BM_BUILD_DIR}/src/, $(notdir ${BM_TEMPLATES_CPP}))
PD_PUBLIC_HEADERS_DIR := ${PUBLIC_INC_PATH}/p4_sim
BM_TENJIN_OUTPUT_PUBLIC_HEADERS := $(addprefix ${PD_PUBLIC_HEADERS_DIR}/, $(notdir ${BM_TEMPLATES_PUBLIC_HEADERS}))
# Output of Tenjin are the following files:
# p4c_bm/templates/src/*.c
# p4c_bm/templates/src/*.h
# p4c_bm/templates/inc/*.h
# p4c_bm_templates/src/*.cpp
# All files are output to ${BM_BUILD_DIR}/src/
BM_TENJIN_OUTPUT := ${BM_TENJIN_OUTPUT_C} ${BM_TENJIN_OUTPUT_H} ${BM_TENJIN_OUTPUT_CPP} ${BM_BUILD_THRIFT_DIR}/p4_pd_rpc.thrift ${BM_TENJIN_OUTPUT_PUBLIC_HEADERS}

BM_TENJIN_OUTPUT_NEWEST := $(shell ls -t ${BM_TENJIN_OUTPUT} 2> /dev/null | head -1)
BM_TENJIN_OUTPUT_OLDEST := $(shell ls -tr ${BM_TENJIN_OUTPUT} 2> /dev/null | head -1)
# For a first-time build, above 2 variables will be empty.
ifeq (${BM_TENJIN_OUTPUT_NEWEST},)
BM_TENJIN_OUTPUT_NEWEST := ${BM_BUILD_SRC_DIR}/pd.c
endif
ifeq (${BM_TENJIN_OUTPUT_OLDEST},)
BM_TENJIN_OUTPUT_OLDEST := bm_tenjin_output_oldest
endif

# Make the oldest Tenjin output depend on P4 program and input templates.
${BM_TENJIN_OUTPUT_OLDEST}: %: $(BM_TEMPLATES) ${P4_INPUT} $(wildcard ${P4_INCLUDES_DIR}/*.p4) $(wildcard ${P4_INCLUDES_DIR}/*.h)
	${BM_SHELL} ${P4_INPUT} ${BM_PARAMS}

# Make all Tenjin output files depend on the oldest Tenjin output file.
BM_TENJIN_DUMMY_TARGETS := $(filter-out ${BM_TENJIN_OUTPUT_OLDEST}, ${BM_TENJIN_OUTPUT})
${BM_TENJIN_DUMMY_TARGETS} : ${BM_TENJIN_OUTPUT_OLDEST}

# Every target that depends on the Tenjin output should just add BM_TENJIN_TARGET to its pre-requisite list.
BM_TENJIN_TARGET := ${BM_TENJIN_OUTPUT_NEWEST}

BM_THRIFT_OUTPUT_CPP := $(addprefix ${BM_BUILD_SRC_DIR}/, p4_pd_rpc_constants.cpp ${P4_PREFIX}.cpp p4_pd_rpc_types.cpp)
BM_THRIFT_OUTPUT_H := $(addprefix ${BM_BUILD_SRC_DIR}/, p4_pd_rpc_constants.h p4_pd_rpc_rpc.h p4_pd_rpc_types.h)
${BM_BUILD_SRC_DIR}/p4_pd_rpc_types.h : ${BM_BUILD_THRIFT_DIR}/p4_pd_rpc.thrift ${BM_TENJIN_TARGET}
	thrift --gen cpp --out ${BM_BUILD_SRC_DIR} $<
${BM_THRIFT_OUTPUT_CPP} : ${BM_BUILD_SRC_DIR}/p4_pd_rpc_types.h

ifdef COVERAGE
COVERAGE_FLAGS := --coverage
endif

BM_OBJS_C := $(addsuffix .o, $(basename $(addprefix ${BM_OBJ_DIR}/, $(notdir ${BM_TENJIN_OUTPUT_C}))))
${BM_OBJS_C} : ${BM_OBJ_DIR}/%.o : ${BM_BUILD_DIR}/src/%.c ${BM_TENJIN_TARGET}
	@echo "    Compiling : bm::$(notdir $<)"
	$(VERBOSE)gcc -o $@ $(COVERAGE_FLAGS) $(DEBUG_FLAGS) $(GLOBAL_INCLUDES) $(GLOBAL_CFLAGS) -I $(PUBLIC_INC_PATH) -MD -c $<

BM_OBJS_CPP := $(addsuffix .o, $(basename $(addprefix ${BM_OBJ_DIR}/, $(notdir ${BM_TENJIN_OUTPUT_CPP}))))
BM_OBJS_CPP += $(addsuffix .o, $(basename $(addprefix ${BM_OBJ_DIR}/, $(notdir ${BM_THRIFT_OUTPUT_CPP}))))
${BM_OBJS_CPP} : ${BM_OBJ_DIR}/%.o : ${BM_BUILD_SRC_DIR}/%.cpp ${BM_TENJIN_TARGET} ${BM_BUILD_SRC_DIR}/${P4_PREFIX}.cpp
	@echo "    Compiling : bm::$(notdir $<)"
	$(VERBOSE)g++ -o $@ $(COVERAGE_FLAGS) $(DEBUG_FLAGS) $(GLOBAL_INCLUDES) -I ${PUBLIC_INC_PATH} -MD -c $<

# Include the auto-generated .d dependency files. gcc/g++ generate the .d file
# when -MD option is used.
-include $(BM_OBJS_C:.o=.d)
-include $(BM_OBJS_CPP:.o=.d)

THRIFT_INPUT_FILE := ${BM_BUILD_THRIFT_DIR}/p4_pd_rpc.thrift
THRIFT_SERVICE_NAME := ${P4_PREFIX}
include ${MAKEFILES_DIR}/thrift-py.mk

${BM_LIB}: ${BM_OBJS_C} ${BM_OBJS_CPP}
	ar -rc $@ $^

.PHONY: bm.a
