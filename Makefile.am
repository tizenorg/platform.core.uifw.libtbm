SUBDIRS =

if HOST_CPU_X86_64
SUBDIRS += src
else
SUBDIRS += src drm_slp
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtbm.pc

