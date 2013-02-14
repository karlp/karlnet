# Makefile for building libopencm3 based projects.
# Based on original example makefiles from libopencm3
# 
# Verbose by default please.
V?=1
BINARY=boxcar
OBJS+=ms_systick.o simrf_plat.o jacks.o

# When finalised, this will be the real board, not discovery
LDSCRIPT = stm32f100x8.ld

OPENCM3_DIR=../../../libs/libopencm3
CFLAGS+=-I../../../libs/simrf/lib
CFLAGS+=-I../../../common
LDFLAGS+=-L../../../libs/simrf/lib -lsimrf
include Makefile.include
