PROJECT_DIR := .

include $(PROJECT_DIR)/makefile_settings
include $(PROJECT_DIR)/makefile_utils

vpath %.d $(DEPENDENCIES_DIR)
vpath %.o $(OBJ)

###############################################################################

srcs_module := ThreadPool.cpp
src_test1   := Test1.cpp
srcs        := $(srcs_module) $(src_test1)

objs_module := $(srcs_module:.cpp=.o)
obj_test1   := $(src_test1:.cpp=.o)

dependencies    := $(addprefix $(DEPENDENCIES_DIR)/, $(srcs:.cpp=.d))
objs_to_compile := $(srcs:.cpp=.o)

###############################################################################

$(dependencies): $(DEPENDENCIES_DIR)/%.d: %.cpp | dir_dpnd
	@$(COMP) -MM -o $@ $< -MT '$*.o' -MT '$*.d' $(INCLUDE_DIRS)

include $(dependencies)

###############################################################################

$(objs_to_compile): %.o: %.cpp %.d
	$(call msg_compile, $*)
	@$(COMP) -o $(OBJ)/$@ $< $(COMP_FLAGS)

compile: $(objs_to_compile)

###############################################################################

test1: dir_bin dir_obj
	$(call msg_build, проекта)
	$(call msg_compile, проекта)
	$(call call_make, ./, compile)
	$(call msg_compile_complete)

	$(call msg_linking)

	@$(COMP) -o $(TARGET_PATH) \
		$(addprefix $(OBJ)/, $(objs_module) $(obj_test1)) $(LINK_FLAGS)
	
	$(call msg_build_complete)

###############################################################################

.PHONY: compile compile_root test1

.DEFAULT_GOAL = test1