#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for backend/partitioning
#
# IDENTIFICATION
#    src/backend/partitioning/Makefile
#
#-------------------------------------------------------------------------

subdir = src/backend/partitioning
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global

OBJS = partbounds.o partdesc.o partprune.o
ifeq ($(enable_cluster), yes)
  OBJS += leafsearch.o
endif

include $(top_srcdir)/src/backend/common.mk
