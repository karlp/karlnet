# Makefile for building libopencm3 based projects.
# Based on original example makefiles from libopencm3
# 
# Verbose by default please.
V?=1
# We use texane/stlink for flashing (via tcp)
STLINK_PORT?=:4242
BINARY=boxcar
OBJS+=ms_systick.o nastylog.o

# When finalised, this will be the real board, not discovery
LDSCRIPT = stm32f100x8.ld
#LDSCRIPT = stm32f100xb.ld  # VL-Discovery
#CFLAGS+=-I../../shared

#CFLAGS+=-I../libs
#LDFLAGS+=-L../libs -lmodbusinoc -lobis

include Makefile.include
