# Install script for directory: /home/rpmbuild/Desktop/cheops-ng-0.2.4/src

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/usr/local")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

# Install shared libraries without execute permission?
IF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  SET(CMAKE_INSTALL_SO_NO_EXE "0")
ENDIF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  IF(EXISTS "$ENV{DESTDIR}/usr/local/bin/cheops-agent" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/cheops-agent")
    FILE(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/local/bin/cheops-agent"
         RPATH "")
  ENDIF()
  list(APPEND CPACK_ABSOLUTE_DESTINATION_FILES
   "/usr/local/bin/cheops-agent")
FILE(INSTALL DESTINATION "/usr/local/bin" TYPE EXECUTABLE FILES "/home/rpmbuild/Desktop/cheops-ng-0.2.4/bin/x86_64-Linux/cheops-agent")
  IF(EXISTS "$ENV{DESTDIR}/usr/local/bin/cheops-agent" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/cheops-agent")
    IF(CMAKE_INSTALL_DO_STRIP)
      EXECUTE_PROCESS(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/local/bin/cheops-agent")
    ENDIF(CMAKE_INSTALL_DO_STRIP)
  ENDIF()
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  IF(EXISTS "$ENV{DESTDIR}/usr/local/bin/cheops-ng" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/cheops-ng")
    FILE(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/local/bin/cheops-ng"
         RPATH "")
  ENDIF()
  list(APPEND CPACK_ABSOLUTE_DESTINATION_FILES
   "/usr/local/bin/cheops-ng")
FILE(INSTALL DESTINATION "/usr/local/bin" TYPE EXECUTABLE FILES "/home/rpmbuild/Desktop/cheops-ng-0.2.4/bin/x86_64-Linux/cheops-ng")
  IF(EXISTS "$ENV{DESTDIR}/usr/local/bin/cheops-ng" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/local/bin/cheops-ng")
    IF(CMAKE_INSTALL_DO_STRIP)
      EXECUTE_PROCESS(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/local/bin/cheops-ng")
    ENDIF(CMAKE_INSTALL_DO_STRIP)
  ENDIF()
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  list(APPEND CPACK_ABSOLUTE_DESTINATION_FILES
   "/usr/local/share/cheops-ng/pixmaps/")
FILE(INSTALL DESTINATION "/usr/local/share/cheops-ng/pixmaps" TYPE DIRECTORY FILES "/home/rpmbuild/Desktop/cheops-ng-0.2.4/src/pixmaps/")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  list(APPEND CPACK_ABSOLUTE_DESTINATION_FILES
   "/usr/local/share/pixmaps/cheops-ng.xpm")
FILE(INSTALL DESTINATION "/usr/local/share/pixmaps" TYPE FILE FILES "/home/rpmbuild/Desktop/cheops-ng-0.2.4/src/pixmaps/cheops-ng.xpm")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  list(APPEND CPACK_ABSOLUTE_DESTINATION_FILES
   "/usr/local/share/applications/cheops-ng.desktop")
FILE(INSTALL DESTINATION "/usr/local/share/applications" TYPE FILE FILES "/home/rpmbuild/Desktop/cheops-ng-0.2.4/src/cheops-ng.desktop")
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

