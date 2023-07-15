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

#set( CHEOPS_BUILD_ARCH "${CMAKE_HOST_SYSTEM_PROCESSOR}-${CMAKE_HOST_SYSTEM_NAME}" )
set( CHEOPS_BUILD_ARCH "x86_64-Linux" )

set( CHEOPS_NAME "cheops-ng" )
set( CHEOPS_AUTHORS "Brent Piddy, FlowerGenius" )
set( CHEOPS_EMAILS "<>, <officialflowergenius@gmail.com>" )
set( CHEOPS_VERSION 0.2.4 )
set( CHEOPS_COPYRIGHT "Copyright (C) 1999, Brent Piddy GNU GPL v2" )
set( CHEOPS_LANGUAGES C )
set( CHEOPS_DESCRIPTION_SHORT
	"Cheops Network User Interface (Next-Generation GUI)"
)

set( CHEOPS_DESCRIPTION_LONG
	""
	""
	""
)

message(STATUS "Started build for ${CHEOPS_NAME} v${CHEOPS_VERSION} on ${CHEOPS_BUILD_ARCH}")
project( ${CHEOPS_NAME} )
#	VERSION ${CHEOPS_VERSION}
#	LANGUAGES ${CHEOPS_LANGUAGES}
#	DESCRIPTION ${CHEOPS_DESCRIPTION_SHORT})

set ( CHEOPS_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src" )
set ( CHEOPS_BINARY_DIR "${CMAKE_CURRENT_LIST_DIR}/bin/${CHEOPS_BUILD_ARCH}" )
set ( CMAKE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}" )

set ( CMAKE_BINARY_DIR "${CHEOPS_BINARY_DIR}" )
set ( CMAKE_CONFIG_DIR "${CMAKE_SOURCE_DIR}/config" )
set ( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CHEOPS_BINARY_DIR}" )

include_directories(PUBLIC SYSTEM "${CMAKE_SOURCE_DIR}/include")

#Get Option Values
option( CHEOPS_C99_EXTENSIONS "Set ON to enable C99 features" ON )
option( CHEOPS_C11_EXTENSIONS "Set ON to enable C11 features" ON )
option( CHEOPS_DEBUG "Set ON to enable debugging" OFF )
option( CMAKE_POSITION_INDEPENDENT_CODE "Set ON to activate -fPIC flag" ON )

#Get GUI Configuration
option( CMAKE_BUILD_GTK+	"Build CHEOPS using GTK libraries" ON )
#option( CMAKE_BUILD_GTK2	"Build CHEOPS using modern GTK Libraries" OFF )
option( CMAKE_BUILD_QT		"Build CHEOPS using QT libraries" OFF )
#option( CMAKE_BUILD_X11	"Build CHEOPS using X11 only" OFF )
option( GNOMELEGACY 			"Build CHEOPS for Legacy GTK Fedora" ON )


#Process Optional Conditions
message(STATUS "Processing Optional Conditions")
	if(CHEOPS_C99_EXTENSIONS)
		set( PROJECT_C_STANDARD 99 )
	endif()

	if(CHEOPS_C11_EXTENSIONS)
		set( PROJECT_C_STANDARD 11 )
	endif()

	set( CHEOPS_C_STANDARD ${PROJECT_C_STANDARD})

# Package Management
message(STATUS "Verifying Dependencies")
	find_package(PkgConfig REQUIRED)

	
	find_package(FLEX)
	if(FLEX_FOUND)
	else()
		find_package(LEX REQUIRED)
	endif()
	
	find_package(BISON)
	if(BISON_FOUND)
	else()
		find_package(YACC REQUIRED)
	endif()
	
	find_package(X11 REQUIRED)
	
	#~ find_package(FREETYPE REQUIRED)
	#~ find_package(GCCXML REQUIRED)
	#~ find_package(OPENSSL REQUIRED)
	find_package(PNG REQUIRED)
	find_package(GIF REQUIRED)
	find_package(JPEG REQUIRED)
	#~ find_package(LibXml2 REQUIRED)
	find_package(TIFF REQUIRED)
	
	#~ find_package(THREADS REQUIRED)
	#~ find_package(IMLIB REQUIRED)
	#find_package(ESOUND REQUIRED)
	#find_package(AUDIOFILE REQUIRED)
	
	#pkg_check_modules(AUDIOFILE REQUIRED audiofile)
	#pkg_check_modules(ESOUND REQUIRED esound)
	link_libraries(${AUDIOFILE_LIBRARIES} ${ESOUND_LIBRARIES})
	
	list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/config")
	#include(FetchContent)
	
	if(GNOMELEGACY)
			find_package(GTK REQUIRED)
			find_package(GNOME REQUIRED)
			find_package(LIBXML REQUIRED)
			find_package(IMLIB REQUIRED)
			#pkg_check_modules(PTHREAD REQUIRED pthread)
			link_libraries(${GNOME_LIBRARIES} ${GTK_glib_LIBRARY} ${GTK_gtk_LIBRARY} ${GTK_gdk_LIBRARY} ${GTK_gmodule_LIBRARY} ${GTK_gthread_LIBRARY} ${IMLIB_LIBRARIES} ${JPEG_jpeg_LIBRARY} ${TIFF_tiff_LIBRARY} ${GIF_gif_LIBRARY} ${PNG_png_LIBRARY} ${LIBXML_LIBRARIES})
	else()
			find_package(GTK REQUIRED)
			find_package(GNOME12 REQUIRED)
			#pkg_check_modules(GNOME2 gnome-2.0 gnomeui-2.0)
			find_package(LIBXML2 REQUIRED)
			find_package(IMLIB2 REQUIRED)
			link_libraries(${GNOME12_LIBRARIES} ${GTK_glib_LIBRARY} ${GTK_gtk_LIBRARY} ${GTK_gdk_LIBRARY} ${GTK_gmodule_LIBRARY} ${GTK_gthread_LIBRARY} ${IMLIB2_LIBRARIES} ${JPEG_jpeg_LIBRARY} ${TIFF_tiff_LIBRARY} ${GIF_gif_LIBRARY} ${PNG_png_LIBRARY} ${LIBXML2_LIBRARIES})
	endif()
	
	if(SOLARIS)
		if(SOLARIS_VERSION GREATER EQUAL 2.6)
			pkg_check_modules(NETLIBS REQUIRED xnet)
		else()
			pkg_check_modules(NETLIBS REQUIRED nsl, socket)
		endif()
		link_libraries(${NETLIBS_LIBRARIES})
	endif()

	
	if(PkgConfig_FOUND)
		add_subdirectory(dependencies)
		message(STATUS "Negotiated Dependencies")
	else()
		message( "Pkg-Config not found, this program may only be installed on a pkg-config controlled system")
	endif()

## Debugging
set(DEBUG_FLAGS "")
if(CHEOPS_DEBUG)
	set(DEBUG_FLAGS "-DDEBUG_DISCOVER -DDEBUG_EVENT -DDEBUG_IO -DDEBUG_SCHEDULER") 
endif()

set(CHEOPS_INCLUDE_DIRECTORIES
	"${GNOME_INCLUDE_DIRS}" 
	"${GTK_glib_INCLUDE_PATH}"
	"${GTK_gtk_INCLUDE_PATH}"
	"${GTK_gdk_INCLUDE_PATH}"
	"${LIBXML_INCLUDE_DIRS}"
	"${CMAKE_CURRENT_LIST_DIR}/adns-1.0/src"
	"${CMAKE_CURRENT_LIST_DIR}/src"
)

message(STATUS "${GNOME_LIBRARIES}")

include_directories(
		${CHEOPS_INCLUDE_DIRECTORIES}
)
add_definitions(
			${DEBUG_FLAGS}
			"-DNEED_GNOMESUPPORT_H"
)


set(CHEOPS_C_CFLAGS "-g -O2 -Wall")
set(CHEOPS_C_LDFLAGS "-L/usr/lib64-rdynamic -L/usr/lib64")


## Flags and Configuration
#set(CMAKE_C_FLAGS "-g -O2 -Wall ${DEBUG_FLAGS} -I/usr/include/gnome-1.0 -DNEED_GNOMESUPPORT_H -I/usr/lib64/gnome-libs/include -I/usr/include/glib-1.2 -I/usr/lib64/glib/include -I/usr/include/orbit-1.0 -I/usr/include/gtk-1.2 -I/usr/include/gtk-1.2 -I/usr/include/glib-1.2 -I/usr/lib64/glib/include -I/usr/include/glib-1.2 -I/usr/lib64/glib/include -I/usr/include/gnome-xml -I${CMAKE_CURRENT_LIST_DIR}/adns-1.0/src")
set(CMAKE_C_FLAGS ${CHEOPS_C_CFLAGS})
set(CMAKE_EXE_LINKER_FLAGS "")
set(CMAKE_INSTALL_PREFIX /usr/local)
set(DEFAULT_PATH ${CMAKE_INSTALL_PREFIX}/share/cheops-ng)

if(DEFAULT_PATH)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEFAULT_PATH=\\\"${DEFAULT_PATH}\\\"")
    #set(CHEOPS_C_FLAGS ${CHEOPS_C_FLAGS} "-DEFAULT_PATH=${DEFAULT_PATH}/" )
endif()
#set(LIBS -lnsl -lcrypt -lpthread -ladns  -lSM -lICE -L/usr/lib64 -lgtk -lgdk -rdynamic -lgmodule -ldl -lglib -lXi -lXext -lX11 -lm -rdynamic  -rdynamic -lgnomeui -lart_lgpl -lgdk_imlib -lSM -lICE -lgtk -lgdk -lgmodule -ldl -lXi -lXext -lX11 -lgnome -lgnomesupport -lesd -laudiofile -lm -lglib -L/usr/lib64 -lgtk -lgdk -rdynamic -lgmodule -ldl -lglib -lXi -lXext -lX11 -lm -L/usr/lib64 -lglib -lImlib -ljpeg -ltiff -lgif -lpng -lm -lXext -lSM -lICE -lXext -lX11 -L/usr/lib64 -lxml)
set(LIBS -lcrypt -lpthread -ladns ${CHEOPS_C_LDFLAGS} )


#configure_file(${CMAKE_CONFIG_DIR}/config.h.in ${CMAKE_SOURCE_DIR}/include/config.h)
configure_file( ${CMAKE_CONFIG_DIR}/cheops-ng.desktop.in 		"${CMAKE_BINARY_DIR}/cheops-ng.desktop" )
configure_file( ${CMAKE_CONFIG_DIR}/cheops-ng.spec.in 			"${CMAKE_SOURCE_DIR}/cheops-ng.spec" )
configure_file( ${CMAKE_CONFIG_DIR}/cheops-ng-static.spec.in 	"${CMAKE_SOURCE_DIR}/cheops-ng-static.spec" )
configure_file( ${CMAKE_CONFIG_DIR}/version.h.in 				"${CMAKE_SOURCE_DIR}/include/version.h" )


include(${CMAKE_CONFIG_DIR}/gpl.cmake)

#add_executable(cheops-agent "")
#add_executable(cheops-ng "")

add_subdirectory("${CMAKE_SOURCE_DIR}/lib")
#add_subdirectory("${CMAKE_SOURCE_DIR}/share")
add_subdirectory("${CMAKE_SOURCE_DIR}/src")
 
#~ end
