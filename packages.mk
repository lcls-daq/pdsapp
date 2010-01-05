ifneq ($(strip $(findstring i386-linux,$(tgt_arch)) \
               $(findstring x86_64-linux,$(tgt_arch))),)
# List of packages (low level first)
packages := tools dev devtest config mon monobs control epics
endif

ifneq ($(findstring ppc-rtems-rce,$(tgt_arch)),)
# List of packages (low level first)
packages := 

endif
