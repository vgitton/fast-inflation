$(info Parsing makefile...)

BIN_DIR := bin

# output binary - debug
DEBUG_BIN := debug_inf
# - release
RELEASE_BIN := release_inf

# source files

SRC_DIR := src

MAIN.cpp := $(SRC_DIR)/main.cpp

# util library

UTIL_DIR := $(SRC_DIR)/util

UTIL_SRCS := \
	math \
	frac \
	debug \
	to_string \
	logger \
	color \
	chrono \
	rand \
	range \
	product_range \
	permutations \
	base_cli \
	file_stream \

UTIL_SRCS := $(patsubst %, $(UTIL_DIR)/%.cpp, $(UTIL_SRCS))

# inf library

INF_DIR := $(SRC_DIR)/inf
	
INF_SRCS := \
	constraints/constraint_parser \
	constraints/constraint_set \
	constraints/dual_vector \
	constraints/marginal \
	constraints/constraint \
	\
	events/event_tensor \
	events/event_tree \
	events/tree_splitter \
	\
	inf_problem/feas_options \
	inf_problem/feas_pb \
	inf_problem/vis_pb \
	inf_problem/inflation \
	inf_problem/network \
	inf_problem/target_distr \
	inf_problem/tree_filler \
	\
	frank_wolfe/frank_wolfe \
	frank_wolfe/fully_corrective_fw \
	frank_wolfe/pairwise_fw \
	\
	optimization/bf_opt \
	optimization/tree_opt \
	optimization/optimizer \
	\
	symmetry/party_sym \
	symmetry/orbitable \
	symmetry/outcome_sym \
	symmetry/symmetry \
	symmetry/tensor_with_orbits \

INF_SRCS := $(patsubst %, $(INF_DIR)/%.cpp, $(INF_SRCS))

# user library

USER_DIR := $(SRC_DIR)/user

USER_SRCS := \
	app_manager \
	application \
	application_list \
	inf_cli \
	\
	applications/misc \
	applications/ejm \
	applications/srb \
	applications/three_ebits \

USER_SRCS := $(patsubst %, $(USER_DIR)/%.cpp, $(USER_SRCS))

SRCS := \
	$(MAIN.cpp) \
	$(UTIL_SRCS) \
	$(INF_SRCS) \
	$(USER_SRCS) \

# DEBUG ================

# intermediate directory for generated object files
DEBUG_DIR := $(BIN_DIR)/debug
DEBUG_MAIN.o := $(patsubst %, $(DEBUG_DIR)/%.o, $(basename $(MAIN.cpp)))
# - same but release
RELEASE_DIR := $(BIN_DIR)/release

DEBUG_LIB_UTIL := $(DEBUG_DIR)/libutil.so
DEBUG_LIB_INF  := $(DEBUG_DIR)/libinf.so
DEBUG_LIB_USER := $(DEBUG_DIR)/libuser.so

# Here the order matters!!!
DEBUG_LIBS := \
	$(DEBUG_MAIN.o) \
	$(DEBUG_LIB_USER) \
	$(DEBUG_LIB_INF) \
	$(DEBUG_LIB_UTIL) \

DEBUG_OBJS_UTIL := $(patsubst %, $(DEBUG_DIR)/%.o, $(basename $(UTIL_SRCS)))
DEBUG_OBJS_INF  := $(patsubst %, $(DEBUG_DIR)/%.o, $(basename $(INF_SRCS)))
DEBUG_OBJS_USER := $(patsubst %, $(DEBUG_DIR)/%.o, $(basename $(USER_SRCS)))

# object files, auto generated from source files
DEBUG_OBJS := \
	$(DEBUG_MAIN.o) \
	$(DEBUG_OBJS_UTIL) \
	$(DEBUG_OBJS_INF) \
	$(DEBUG_OBJS_USER)

# RELEASE =================

RELEASE_OBJS := $(patsubst %, $(RELEASE_DIR)/%.o, $(basename $(SRCS)))

# dependency files, auto generated from source files
DEBUG_DEPS   := $(patsubst %, $(DEBUG_DIR)/%.d, $(basename $(SRCS)))
RELEASE_DEPS := $(patsubst %, $(RELEASE_DIR)/%.d, $(basename $(SRCS)))

# C++ compiler
CXX := g++
# linker
LD := g++

MOSEK := $${HOME}/mosek/mosek/10.0/tools/platform/linux64x86
MOSEK_INCLUDE := $(MOSEK)/h
MOSEK_BIN_DIR := $(MOSEK)/bin
MOSEK_LIBS := $(MOSEK_BIN_DIR)/libfusion64.so.10.0 $(MOSEK_BIN_DIR)/libmosek64.so.10.0
GMP_LIB := libgmp.a

CXX_FLAGS := \
	-std=c++2a \
	-fdiagnostics-color=always \
	-Wall \
	-Wextra \
	-pedantic \
	-Wold-style-cast \
	-Wconversion \
	-Wunreachable-code \
	-Wno-stringop-overread \
	-isystem$(MOSEK_INCLUDE) \

# Debug flags
DEBUG_FLAGS := \
	-fPIC \
	-D_GLIBCXX_DEBUG \
	#-g # <- put back for gdb use

# Release flags
RELEASE_FLAGS := \
	-DNDEBUG \
	-O3 \

# flags required for dependency generation; passed to compilers
DEBUG_DEP_FLAGS   = -MT $@ -MMD -MP -MF $(DEBUG_DIR)/$*.Td
RELEASE_DEP_FLAGS = -MT $@ -MMD -MP -MF $(RELEASE_DIR)/$*.Td

# compile C++ source files
DEBUG_COMPILE = @$(CXX) $(DEBUG_DEP_FLAGS) $(CXX_FLAGS) $(DEBUG_FLAGS) -c -o $@
RELEASE_COMPILE = @$(CXX) $(RELEASE_DEP_FLAGS) $(CXX_FLAGS) $(RELEASE_FLAGS) -c -o $@

# Create shared library
CREATE_SO = @echo -n 'Creating shared library $@... ' && $(CXX) -shared -o $@ $^
# link object files to binary
# In debug mode, $(MOSEK_LIBS) will already be linked to the $(DEBUG_LIB_INF) shared library
# -rpath=. adds the current directory to the runtime library search path
DEBUG_LINK.o = @echo -n 'Linking shared libraries... ' && $(LD) -Wl,-rpath,$(shell pwd) -o $@ $^ -l:$(GMP_LIB) -pthread && echo ✓
RELEASE_LINK.o = @echo -n 'Linking... ' && $(LD) -o $@ $^ $(MOSEK_LIBS) -l:$(GMP_LIB) -pthread && echo ✓
# postcompile step
DEBUG_POSTCOMPILE   = @mv -f $(DEBUG_DIR)/$*.Td $(DEBUG_DIR)/$*.d
RELEASE_POSTCOMPILE = @mv -f $(RELEASE_DIR)/$*.Td $(RELEASE_DIR)/$*.d

.PHONY: d debug r release doc clean prepare

d: debug
debug: $(DEBUG_BIN) 

r: release
release: $(RELEASE_BIN) 

# Note: the variables RUN_ARGS no longer exists,
# but leaving this bit of code in nonetheless
# to remember how to call gdb.
# .PHONY: gdb
# gdb: $(DEBUG_BIN)
# 	@echo Running $(strip $(RUN_ARGS) in) gdb...
# 	@gdb -q --args $< $(RUN_ARGS)

$(RELEASE_BIN): $(RELEASE_OBJS) 
	$(RELEASE_LINK.o)

$(DEBUG_BIN): $(DEBUG_LIBS)
	$(DEBUG_LINK.o)

# Creating the shared libraries
$(DEBUG_LIB_UTIL): $(DEBUG_OBJS_UTIL)
	$(CREATE_SO)
	@echo ✓
$(DEBUG_LIB_INF): $(DEBUG_OBJS_INF) 
	$(CREATE_SO) $(MOSEK_LIBS)
	@echo ✓
$(DEBUG_LIB_USER): $(DEBUG_OBJS_USER)
	$(CREATE_SO)
	@echo ✓

$(RELEASE_DIR)/%.o: %.cpp
$(RELEASE_DIR)/%.o: %.cpp $(RELEASE_DIR)/%.d
	@echo -n "Compiling $<... "
	$(RELEASE_COMPILE) $<
	$(RELEASE_POSTCOMPILE)
	@echo ✓

# Test

$(DEBUG_DIR)/%.o: %.cpp
$(DEBUG_DIR)/%.o: %.cpp $(DEBUG_DIR)/%.d
	@echo -n "Compiling $<... "
	$(DEBUG_COMPILE) $<
	$(DEBUG_POSTCOMPILE)
	@echo ✓

.PRECIOUS: $(DEBUG_DIR)/%.d
$(DEBUG_DIR)/%.d: ;
.PRECIOUS: $(RELEASE_DIR)/%.d
$(RELEASE_DIR)/%.d: ;

-include $(DEBUG_DEPS)
-include $(RELEASE_DEPS)

# May need to change this one
DOXYGEN := /usr/local/bin/doxygen
DOXYGEN_CONFIGFILE := doxygen/doxygen.configfile
doc:
	@echo Creating documentation...
	@$(DOXYGEN) $(DOXYGEN_CONFIGFILE) 2>&1 1> /dev/null | perl -pe "s/(.+?) /\033[30;1m\1\033[0m\n/" | perl -pe "s/warning:/\033[33mWarning:\033[0m/"
	@echo Done.

# compilers (at least gcc and clang) don't create the subdirectories automatically
prepare:
	@echo Creating bin/ subdirectory structure...
	@mkdir -p $(dir $(DEBUG_OBJS))
	@mkdir -p $(dir $(RELEASE_OBJS)) >/dev/null
	@mkdir -p $(dir $(DEBUG_DEPS)) >/dev/null
	@mkdir -p $(dir $(RELEASE_DEPS)) >/dev/null

clean:
	$(RM) -r $(BIN_DIR)
	$(RM) $(DEBUG_BIN) $(RELEASE_BIN)

$(info Done.)

