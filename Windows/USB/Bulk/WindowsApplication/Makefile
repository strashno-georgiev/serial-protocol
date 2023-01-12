# Makefile for Linux


all: Exe/Test

INCDIR = ../Inc
API    = ../USBBULK_API_Linux/USBBULK_Linux.c
HDR    = $(INCDIR)/USBBULK.h
LIBS   = -ludev -lpthread

Exe/Test: Src/Test.c $(API) $(HDR)
	cc -Wall -O -I$(INCDIR) $^ -o $@ $(LIBS)

