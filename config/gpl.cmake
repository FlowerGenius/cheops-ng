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

message(STATUS "Configuring Copyright Information Source File (gpl.c)")

set(GPL ${PROJECT_SOURCE_DIR}/src/gpl.c)

#execute_process(
#	COMMAND ${PROJECT_SOURCE_DIR}/config/gpl.sh ${PROJECT_SOURCE_DIR}
#	OUTPUT_FILE "${GPL}"
#)

configure_file( ${PROJECT_SOURCE_DIR}/config/gpl.c.in ${GPL} )
