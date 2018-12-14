# Use this script to define cc::comms and cc::comms_champion targets
# when compiling CommsChampion using ExternalProject_Add() function
#
# Expected inputs, set the following variables before including this file
# EXT_CC_INSTALL_DIR - Future CommsChampion install directory
# EXT_CC_NO_COMMS_CHAMPION - Bool value to explude comms_champion library target setup.
# EXT_QT_DIR - Directory of Qt5 installation
#
# Output is provided using the following variables
#  CC_COMMS_FOUND - Found COMMS library, use CC_INCLUDE_DIRS for includes
#  CC_COMMS_CHAMPION_FOUND - Found CommsChampion, use CC_INCLUDE_DIRS, CC_PLUGIN_LIBRARIES, CC_PLUGIN_DIR)
#  CC_INCLUDE_DIRS - Where to find all the headers
#  CC_ROOT_DIR - Root directory of CommsChampion installation
#  CC_PLUGIN_LIBRARIES - Libraries (comms_champion) to link against when building a plugin
#  CC_PLUGIN_LIBRARY_DIRS - Where to find the libraries required to build plugin.
#  CC_PLUGIN_DIR - Directory where to install newly built plugin
#
# Additional output targets
# cc::comms - Link target for COMMS library
# cc::comms_champion - Link target for "comms_champion" libarary. Use it in plugin development.

if (NOT EXT_CC_INSTALL_DIR)
    message (FATAL_ERROR "Future CommsChampion installation directory is not provided")
endif ()

set (CC_ROOT_DIR ${EXT_CC_INSTALL_DIR})

set (CC_COMMS_FOUND TRUE)
set (CC_INCLUDE_DIRS "${EXT_CC_INSTALL_DIR}/include")

add_library(comms INTERFACE)
add_library(cc::comms ALIAS comms)
target_include_directories(comms INTERFACE ${CC_INCLUDE_DIRS})

target_compile_options(comms INTERFACE
  $<$<CXX_COMPILER_ID:MSVC>:/wd4503 /wd4309 /wd4267 -D_SCL_SECURE_NO_WARNINGS>
)

if (EXT_CC_NO_COMMS_CHAMPION)
    return ()
endif ()

find_package(Qt5Core)
find_package(Qt5Widgets)

if (WIN32)
    find_library(qt5platformsupport_rel qt5platformsupport HINTS "${EXT_QT_DIR}/lib")
    find_library(qt5platformsupport_deb qt5platformsupportd HINTS "${EXT_QT_DIR}/lib")

    set (cc_platform_specific)
    if (qt5platformsupport_rel AND qt5platformsupport_deb)
        add_library(qt5platformsupport_lib UKNOWN IMPORTED)
        set_target_properties(qt5platformsupport PROPERTIES IMPORTED_LOCATION_DEBUG ${qt5platformsupport_rel})
        set_target_properties(qt5platformsupport PROPERTIES IMPORTED_LOCATION_RELEASE ${qt5platformsupport_deb})
        list (APPEND cc_platform_specific qt5platformsupport_lib)
    else ()
        message(WARING "Platform support is missing!")
    endif ()

    list (APPEND cc_platform_specific Setupapi.lib Ws2_32.lib opengl32.lib imm32.lib winmm.lib)
endif ()   

set (CC_COMMS_CHAMPION_FOUND TRUE)
set (CC_PLUGIN_LIBRARIES "comms_champion")
set (CC_PLUGIN_LIBRARY_DIRS ${EXT_CC_INSTALL_DIR}/lib)
set (CC_PLUGIN_DIR ${EXT_CC_INSTALL_DIR}/lib/CommsChampion/plugin)

if (WIN32)
    set (cc_lib_name ${CC_PLUGIN_LIBRARIES}.lib)
else ()
    set (cc_lib_name lib${CC_PLUGIN_LIBRARIES}.so)
endif ()

add_library(cc::comms_champion UNKNOWN IMPORTED)
file (MAKE_DIRECTORY ${CC_INCLUDE_DIRS})

set (interface_link_libs cc::comms)
if (Qt5Widgets_FOUND)
    list (APPEND interface_link_libs Qt5::Widgets)
endif ()

if (Qt5Core_FOUND)
    list (APPEND interface_link_libs Qt5::Core)
endif ()

if (cc_platform_specific)
    list (APPEND interface_link_libs ${cc_platform_specific})
endif ()

set_target_properties(cc::comms_champion PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${CC_INCLUDE_DIRS}
    IMPORTED_LOCATION ${CC_PLUGIN_LIBRARY_DIRS}/${cc_lib_name}
    INTERFACE_LINK_LIBRARIES "${interface_link_libs}"
)



