# This is an automatically generated record.
# The area between QNX Internal Start and QNX Internal End is controlled by
# the QNX IDE properties.

ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

#===== USEFILE - the file containing the usage message for the application. 
USEFILE=

#===== EXTRA_INCVPATH - a space-separated list of directories to search for include files.
EXTRA_INCVPATH+=$(PROJECT_ROOT)/src  \
	$(PROJECT_ROOT)/src/osdep $(PROJECT_ROOT)/src/robot

#===== EXTRA_SRCVPATH - a space-separated list of directories to search for source files.
EXTRA_SRCVPATH+=$(PROJECT_ROOT)/src  \
	$(PROJECT_ROOT)/src/robot

#===== LIBS - a space-separated list of library items to be included in the link.
LIBS+=m socks socket

include $(MKFILES_ROOT)/qmacros.mk
#===== EXTRA_OBJS - object files and libraries contributed additionaly on link/archive step
EXTRA_OBJS+=$(PROJECT_ROOT)/src/sqlite3_so/libsqlite3.so

ifndef QNX_INTERNAL
QNX_INTERNAL=$(PROJECT_ROOT)/.qnx_internal.mk
endif
include $(QNX_INTERNAL)

include $(MKFILES_ROOT)/qtargets.mk

OPTIMIZE_TYPE_g=none
OPTIMIZE_TYPE=$(OPTIMIZE_TYPE_$(filter g, $(VARIANTS)))

