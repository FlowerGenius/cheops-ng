#=cheops-ng====================================================================
# Name		:
# Author	:
# Version	:
# Copyright	:
# Description	:
# Module	:
# Created	:
# Modified	:
#==============================================================================

cmake_minimum_required(VERSION 2.8)

add_executable(cheops-ng  ${COMOBJS} ${GUIOBJS} ${GTKOBJS} ${NORMOBJS} io-gk.c)