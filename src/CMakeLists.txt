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

# Common Objects
set(COMOBJS logger.c misc.c cerror.c cache.c script.c)

# Used by the agent and sh
set(NORMOBJS sched.c cheops-main.c)

# Used by the gui
set(GTKOBJS gui-io.c gui-sched.c event.c gui-help.c gpl.c)

# Objects for Cheops Agent
set(AGENTOBJS cheops-agent.c agent-discover.c agent-settings.c event.c io.c ip_utils.c agent-osscan.c agent-map.c probe.c agent-probe.c agent-auth.c agent-nmapxml.c)

set(AGENTLIBS ${CMAKE_SOURCE_DIR}/adns-1.0/src/libadns.a)
set(AGENTLIBFLAGS -lm ${LIBS} -L${CMAKE_SOURCE_DIR}/adns-1.0/src/ -lcrypt)

# Objects for Cheops Agent Shell
set(SHOBJS event.c io.c cheops-sh.c cheops-sh-guts.c ip_utils.c)
set(SHLIBS -lreadline -lncurses -lm -lcrypt)

# Objects for Cheops UI
set(GUIOBJS cheops-gui.c gui-handlers.c gui-viewspace.c gui-settings.c gui-canvas.c gui-utils.c gui-config.c ip_utils.c cheops-osscan.c gui-dns.c gui-service.c gui-pixmap.c gui-monitoring.c cheops-monitoring.c)

# Additional programs
set(TEST_OBJS ${NORMOBJS} agent-io.o logger.o misc.o)
set(TESTS test-sched test-socket test-probe)
set(COMLIBS) #@COMLIBS@

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g -I${CMAKE_SOURCE_DIR}/pixmaps -I${CMAKE_SOURCE_DIR}/adns-1.0/src ${DEBUG_FLAGS}")
if (CHEOPS_DEBUG)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEBUG_DISCOVER -DDEBUG_MAP -DDEBUG_NMAPXML -DDEBUG_OS_SCAN -DDEBUG_AGENT_PROBE -DDEBUG_CHEOPS_GUI -DDEBUG_EVENT -DDEBUG_CANVAS -DDEBUG_CONFIG -DDEBUG_GUI_HANDLERS -DDEBUG_IO -DDEBUG_PROBE")
	target_compile_definitions( ${PROJECT_NAME}
		PUBLIC
			DEBUG_DISCOVER
			DEBUG_MAP
			DEBUG_NMAPXML
			DEBUG_OS_SCAN
			DEBUG_AGENT_PROBE
			DEBUG_CHEOPS_GUI
			DEBUG_EVENT
			DEBUG_CANVAS
			DEBUG_CONFIG
			DEBUG_GUI
			DEBUG_GUI_HANDLERS
			DEBUG_IO
			DEBUG_PROBE
	)		
endif()

#set(REVISION `cat version`)
#set(PROGS cheops-agent cheops-ng)

add_executable(cheops-agent ${AGENTOBJS} ${AGENTLIBS} ${COMOBJS} ${NORMOBJS})
add_executable(cheops-ng  ${COMOBJS} ${GUIOBJS} ${GTKOBJS} ${NORMOBJS} io-gk.c)

#target_sources(cheops-agent PRIVATE ${AGENTOBJS} ${AGENTLIBS} ${COMOBJS} ${NORMOBJS})
target_link_libraries(cheops-agent ${AGENTLIBS} ${LIBS})

#target_sources(cheops-agent PRIVATE ${COMOBJS} ${GUIOBJS} ${GTKOBJS} ${NORMOBJS} io-gk.c)
target_link_libraries(cheops-ng ${LIBS} ${GTKLIBS})

# Additional programs
if (BUILD_TESTS)
    foreach (test ${TESTS})
        add_executable(${test} ${TEST_OBJS} ${test}.c)
        target_link_libraries(${test} ${COMLIBS})
    endforeach()
endif()

install(TARGETS cheops-agent cheops-ng
        DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(DIRECTORY pixmaps/
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/cheops-ng/pixmaps)
install(FILES pixmaps/cheops-ng.xpm
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pixmaps)
install(FILES cheops-ng.desktop
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
