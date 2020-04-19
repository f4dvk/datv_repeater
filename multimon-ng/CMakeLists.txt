cmake_minimum_required( VERSION 2.8.8 )

# The "MSVC" flag isn't set until the "project" command
# is called.  Let's just check the operating system.

project( multimon-ng C )

set( TARGET "${PROJECT_NAME}" )
set( VERSION "1.1.8" )
set( MAJOR "1" )
set( MINOR "1" )
set( PATCH "8" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra" )

find_package( PulseAudio )
if ( PULSEAUDIO_FOUND )
	option( PULSE_AUDIO_SUPPORT "Enable pulse audio support" ${PULSEAUDIO_FOUND} )
	mark_as_advanced( PULSE_AUDIO_SUPPORT )
endif( PULSEAUDIO_FOUND )

# Check if we can use the GCC/llvm __builtin_popcount
include( CheckCSourceCompiles )
check_c_source_compiles(
	"int main() { __builtin_popcount(42); return 0; }" USE_BUILTIN_POPCOUNT )

set( INSTALL_MAN_DIR "${CMAKE_INSTALL_PREFIX}/share/man" )
install( FILES multimon-ng.1 DESTINATION "${INSTALL_MAN_DIR}/man1" )

if( PULSE_AUDIO_SUPPORT )
	include_directories( ${PULSEAUDIO_INCLUDE_DIR} )
	find_library( PULSE_SIMPLE NAMES pulse-simple REQUIRED )
	link_libraries( ${PULSEAUDIO_LIBRARY} ${PULSE_SIMPLE} )
	add_definitions( "-DPULSE_AUDIO" )
else( PULSE_AUDIO_SUPPORT )
	add_definitions( "-DDUMMY_AUDIO" )
endif( PULSE_AUDIO_SUPPORT )

add_definitions( "-std=gnu99" )
add_definitions( "-DMAX_VERBOSE_LEVEL=3" "-DCHARSET_UTF8" )

set( HEADERS ${HEADERS}
    	multimon.h
    	filter.h
      jetsonGPIO.h
	)

set( SOURCES ${SOURCES}
	unixinput.c
	demod_dtmf.c
	costabi.c
	costabf.c
	jetsonGPIO.c
	)


add_executable( "${TARGET}" ${SOURCES} ${HEADERS} )
set_property(TARGET "${TARGET}" PROPERTY LINKER_LANGUAGE C)
target_link_libraries( "${TARGET}" m )
install(TARGETS multimon-ng DESTINATION bin)
