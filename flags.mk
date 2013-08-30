include $(RELEASE_DIR)/make/sw/flags.mk
include $(RELEASE_DIR)/make/sw/qtflags.mk

false :=
true  := t
#build_extra := $(true)
build_extra := $(false)

CPPFLAGS += -DPsddlPds=Pds
