# List of packages (low level first)
ifneq ($(findstring i386-linux,$(tgt_arch)),)
packages := tools config dev devtest monobs control epics blv bldIpimb test camrecord
endif

ifneq ($(findstring x86_64,$(tgt_arch)),)
packages := tools config dev devtest mon monobs control python epics blv bldIpimb test camrecord
endif

ifneq ($(findstring x86_64-rhel7,$(tgt_arch)),)
packages := tools config mon monobs control epics python test
endif

ifneq ($(findstring ppc-rtems-rce,$(tgt_arch)),)
# List of packages (low level first)
packages := 
endif

#
#  LCLS-II development
#
packages := dev
