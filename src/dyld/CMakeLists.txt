project(dyld)

cmake_minimum_required(VERSION 2.4.0)

if (DEBUG)
	# LLVM for UndefinedFunction
	execute_process(COMMAND "llvm-config" "--cxxflags" RESULT_VARIABLE EXITCODE
		OUTPUT_VARIABLE LLVM_CXXFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)

	if (${EXITCODE})
		message(FATAL_ERROR "LLVM dev headers not found")
	endif (${EXITCODE})

	execute_process(COMMAND "llvm-config" "--ldflags" RESULT_VARIABLE EXITCODE
		OUTPUT_VARIABLE LLVM_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)

	execute_process(COMMAND "llvm-config" "--libs" "engine" RESULT_VARIABLE EXITCODE
		OUTPUT_VARIABLE LLVM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
endif (DEBUG)

MESSAGE("${LLVM_CXXFLAGS}")
MESSAGE("${LLVM_LDFLAGS}")


if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

set(dyld_SRCS
	FileMap.cpp
	MachOLoader.cpp
	Trampoline.cpp
	trampoline_helper.nasm
	dyld_stub_binder.nasm
	ld.cpp
	dyld.cpp
	public.cpp
	binfmt_misc.cpp
)

if (DEBUG)
	set(dyld_SRCS
		${dyld_SRCS}
		UndefinedFunction.cpp
	)

	SET_SOURCE_FILES_PROPERTIES(UndefinedFunction.cpp PROPERTIES
		COMPILE_FLAGS "${LLVM_CXXFLAGS}")

endif (DEBUG)

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib${SUFFIX}/darling")
#SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--enable-new-dtags")
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) 
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_executable(dyld${SUFFIX} ${dyld_SRCS})
target_link_libraries(dyld${SUFFIX} -ldl -lpthread mach-o util ${LLVM_LDFLAGS}
	${LLVM_LIBS})

install(TARGETS dyld${SUFFIX} DESTINATION bin)

