# -*- Mode: makefile-gmake -*-

.PHONY: clean all debug release coverage
.PHONY: debug_lib release_lib coverage_lib

#
# Real test makefile defines EXE (and possibly SRC) and includes this one.
#

ifndef EXE
${error EXE not defined}
endif

SRC ?= $(EXE).c
COMMON_SRC ?= \
  test_gbinder_client.c \
  test_gbinder_local_object.c \
  test_gbinder_local_request.c \
  test_gbinder_local_reply.c \
  test_gbinder_reader_writer.c \
  test_gbinder_remote_object.c \
  test_gbinder_remote_request.c \
  test_gbinder_remote_reply.c \
  test_gbinder_servicemanager.c \
  test_main.c

#
# Required packages
#

LINK_PKGS += libglibutil glib-2.0 gobject-2.0
PKGS += $(LINK_PKGS) libgbinder

#
# Default target
#

all: debug release

#
# Directories
#

SRC_DIR = .
LIB_DIR = ../..
COMMON_DIR = ../common
BUILD_DIR = build
DEBUG_BUILD_DIR = $(BUILD_DIR)/debug
RELEASE_BUILD_DIR = $(BUILD_DIR)/release
COVERAGE_BUILD_DIR = $(BUILD_DIR)/coverage
COMMON_BUILD_DIR = $(COMMON_DIR)/build
COMMON_DEBUG_BUILD_DIR = $(COMMON_BUILD_DIR)/debug
COMMON_RELEASE_BUILD_DIR = $(COMMON_BUILD_DIR)/release
COMMON_COVERAGE_BUILD_DIR = $(COMMON_BUILD_DIR)/coverage

#
# Tools and flags
#

CC ?= $(CROSS_COMPILE)gcc
LD = $(CC)
WARNINGS += -Wall -Wno-deprecated-declarations
INCLUDES += -I$(COMMON_DIR) -I$(LIB_DIR)/src -I$(LIB_DIR)/include
BASE_FLAGS = -fPIC
BASE_LDFLAGS = $(BASE_FLAGS) $(LDFLAGS)
BASE_CFLAGS = $(BASE_FLAGS) $(CFLAGS)
FULL_CFLAGS = $(BASE_CFLAGS) $(DEFINES) $(WARNINGS) $(INCLUDES) -MMD -MP \
  $(shell pkg-config --cflags $(PKGS))
FULL_LDFLAGS = $(BASE_LDFLAGS)
LIBS = $(shell pkg-config --libs $(LINK_PKGS)) -lpthread
QUIET_MAKE = make --no-print-directory
DEBUG_FLAGS = -g
RELEASE_FLAGS =
COVERAGE_FLAGS = -g

DEBUG_LDFLAGS = $(FULL_LDFLAGS) $(DEBUG_FLAGS)
RELEASE_LDFLAGS = $(FULL_LDFLAGS) $(RELEASE_FLAGS)
COVERAGE_LDFLAGS = $(FULL_LDFLAGS) $(COVERAGE_FLAGS) --coverage

DEBUG_CFLAGS = $(FULL_CFLAGS) $(DEBUG_FLAGS) -DDEBUG
RELEASE_CFLAGS = $(FULL_CFLAGS) $(RELEASE_FLAGS) -O2
COVERAGE_CFLAGS = $(FULL_CFLAGS) $(COVERAGE_FLAGS) --coverage

DEBUG_LIB_FILE := $(shell $(QUIET_MAKE) -C $(LIB_DIR) print_debug_lib)
RELEASE_LIB_FILE := $(shell $(QUIET_MAKE) -C $(LIB_DIR) print_release_lib)
COVERAGE_LIB_FILE := $(shell $(QUIET_MAKE) -C $(LIB_DIR) print_coverage_lib)

DEBUG_LIB = $(LIB_DIR)/$(DEBUG_LIB_FILE)
RELEASE_LIB = $(LIB_DIR)/$(RELEASE_LIB_FILE)
COVERAGE_LIB = $(LIB_DIR)/$(COVERAGE_LIB_FILE)

DEBUG_LIBS = $(DEBUG_LIB) $(LIBS)
RELEASE_LIBS = $(RELEASE_LIB) $(LIBS)
COVERAGE_LIBS = $(COVERAGE_LIB) $(LIBS)

#
# Files
#

TEST_DEBUG_OBJS = $(SRC:%.c=$(DEBUG_BUILD_DIR)/%.o)
TEST_RELEASE_OBJS = $(SRC:%.c=$(RELEASE_BUILD_DIR)/%.o)
TEST_COVERAGE_OBJS = $(SRC:%.c=$(COVERAGE_BUILD_DIR)/%.o)
COMMON_DEBUG_OBJS = $(COMMON_SRC:%.c=$(COMMON_DEBUG_BUILD_DIR)/%.o)
COMMON_RELEASE_OBJS = $(COMMON_SRC:%.c=$(COMMON_RELEASE_BUILD_DIR)/%.o)
COMMON_COVERAGE_OBJS = $(COMMON_SRC:%.c=$(COMMON_COVERAGE_BUILD_DIR)/%.o)
DEBUG_OBJS = $(COMMON_DEBUG_OBJS) $(TEST_DEBUG_OBJS)
RELEASE_OBJS = $(COMMON_RELEASE_OBJS) $(TEST_RELEASE_OBJS)
COVERAGE_OBJS = $(COMMON_COVERAGE_OBJS) $(TEST_COVERAGE_OBJS)

#
# Dependencies
#

DEPS = $(DEBUG_OBJS:%.o=%.d) $(RELEASE_OBJS:%.o=%.d)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(DEPS)),)
-include $(DEPS)
endif
endif

$(DEBUG_LIB): | debug_lib
$(RELEASE_LIB): | release_lib
$(COVERAGE_LIB): | coverage_lib

$(TEST_DEBUG_OBJS): | $(DEBUG_BUILD_DIR)
$(TEST_RELEASE_OBJS): | $(RELEASE_BUILD_DIR)
$(TEST_COVERAGE_OBJS): | $(COVERAGE_BUILD_DIR)

$(COMMON_DEBUG_OBJS): | $(COMMON_DEBUG_BUILD_DIR)
$(COMMON_RELEASE_OBJS): | $(COMMON_RELEASE_BUILD_DIR)
$(COMMON_COVERAGE_OBJS): | $(COMMON_COVERAGE_BUILD_DIR)

#
# Rules
#

DEBUG_EXE = $(DEBUG_BUILD_DIR)/$(EXE)
RELEASE_EXE = $(RELEASE_BUILD_DIR)/$(EXE)
COVERAGE_EXE = $(COVERAGE_BUILD_DIR)/$(EXE)

debug: debug_lib $(DEBUG_EXE)

release: release_lib $(RELEASE_EXE)

coverage: coverage_lib $(COVERAGE_EXE)

unitclean:
	rm -f *~
	rm -fr $(BUILD_DIR) $(COMMON_BUILD_DIR) 

clean: unitclean

cleaner: unitclean
	@make -C $(LIB_DIR) clean

test_banner:
	@echo "===========" $(EXE) "=========== "

test: test_banner debug 
	@$(DEBUG_EXE)

valgrind: test_banner debug
	@G_DEBUG=gc-friendly G_SLICE=always-malloc valgrind --tool=memcheck --leak-check=full --show-possibly-lost=no $(DEBUG_EXE)

$(DEBUG_BUILD_DIR):
	mkdir -p $@

$(RELEASE_BUILD_DIR):
	mkdir -p $@

$(COVERAGE_BUILD_DIR):
	mkdir -p $@

$(COMMON_DEBUG_BUILD_DIR):
	mkdir -p $@

$(COMMON_RELEASE_BUILD_DIR):
	mkdir -p $@

$(COMMON_COVERAGE_BUILD_DIR):
	mkdir -p $@

$(DEBUG_BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c $(DEBUG_CFLAGS) -MT"$@" -MF"$(@:%.o=%.d)" $< -o $@

$(RELEASE_BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c $(RELEASE_CFLAGS) -MT"$@" -MF"$(@:%.o=%.d)" $< -o $@

$(COVERAGE_BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) -c $(COVERAGE_CFLAGS) -MT"$@" -MF"$(@:%.o=%.d)" $< -o $@

$(COMMON_DEBUG_BUILD_DIR)/%.o : $(COMMON_DIR)/%.c
	$(CC) -c $(DEBUG_CFLAGS) -MT"$@" -MF"$(@:%.o=%.d)" $< -o $@

$(COMMON_RELEASE_BUILD_DIR)/%.o : $(COMMON_DIR)/%.c
	$(CC) -c $(RELEASE_CFLAGS) -MT"$@" -MF"$(@:%.o=%.d)" $< -o $@

$(COMMON_COVERAGE_BUILD_DIR)/%.o : $(COMMON_DIR)/%.c
	$(CC) -c $(COVERAGE_CFLAGS) -MT"$@" -MF"$(@:%.o=%.d)" $< -o $@

$(DEBUG_EXE): $(DEBUG_LIB) $(DEBUG_OBJS)
	$(LD) $(DEBUG_LDFLAGS) $(DEBUG_OBJS) $(DEBUG_LIBS) -o $@

$(RELEASE_EXE): $(RELEASE_LIB) $(RELEASE_OBJS)
	$(LD) $(RELEASE_LDFLAGS) $(RELEASE_OBJS) $(RELEASE_LIBS) -o $@

$(COVERAGE_EXE): $(COVERAG_LIB) $(COVERAGE_OBJS)
	$(LD) $(COVERAGE_LDFLAGS) $(COVERAGE_OBJS) $(COVERAGE_LIBS) -o $@

debug_lib:
	$(MAKE) -C $(LIB_DIR) $@

release_lib:
	$(MAKE) -C $(LIB_DIR) $@

coverage_lib:
	$(MAKE) -C $(LIB_DIR) $@
