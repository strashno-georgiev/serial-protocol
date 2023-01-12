# Makefile for Linux


all: Exe/Echo

LIBS   = -ludev

Exe/Echo: Application/Echo.c Application/SYS_Linux.c Application/SYS.h
	cc -Wall -O -IApplication $^ -o $@ $(LIBS)
