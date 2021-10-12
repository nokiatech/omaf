
#
# This file is part of Nokia OMAF implementation
#
# Copyright (c) 2018-2021 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
#
# Contact: omaf@nokia.com
#
# This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
# subsidiaries. All rights are reserved.
#
# Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
# written consent of Nokia.
#

cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

#for android .. https://github.com/taka-no-me/android-cmake

project(SDK)
set(CMAKE_DEBUG_POSTFIX "d")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_CONFIGURATION_TYPES Debug Release)

option(ENABLE_MEMORY_TRACKING "Enable memory tracking" OFF)

option(ENABLE_GRAPHICS_API_NIL "Build support for NULL render backend" ON)
option(ENABLE_GRAPHICS_API_D3D11 "Build support for D3D11 render backend" OFF)
option(ENABLE_GRAPHICS_API_OPENGL "Build support for OpenGL render backend" OFF)
option(ENABLE_GRAPHICS_API_OPENGL_ES "Build support for OpenGL ES render backend" OFF)

add_definitions(-DUSE_FIRST_OMAF_HEIF_GITHUB_RELEASE)

if (MSVC)

	set(ENABLE_GRAPHICS_API_D3D11 ON)
	set(ENABLE_GRAPHICS_API_OPENGL OFF)
	
	add_definitions(-DOMAF_AUDIO_BACKEND_WASAPI)

elseif (ANDROID)

	set(ENABLE_GRAPHICS_API_D3D11 OFF)
	set(ENABLE_GRAPHICS_API_OPENGL_ES ON)

else()

	message(FATAL_ERROR "Could not autodetect target OS.")

endif()


################ Set graphics backend flags ################################
if (ENABLE_GRAPHICS_API_NIL)
    add_definitions(-DOMAF_GRAPHICS_API_NIL)
endif()

if (ENABLE_GRAPHICS_API_D3D11)
    add_definitions(-DOMAF_GRAPHICS_API_D3D11)
endif()

if (ENABLE_GRAPHICS_API_OPENGL)

    if (MSVC)

        add_definitions(-DOMAF_GRAPHICS_API_OPENGL=43) # OpenGL 4.3 enabled due to R&D compute shader support

    else()

        message(FATAL_ERROR "Could not determine OpenGL version for target platform")

    endif()

endif()

if (ENABLE_GRAPHICS_API_OPENGL_ES)

    if (ANDROID)

        add_definitions(-DOMAF_GRAPHICS_API_OPENGL_ES=31) # Android supports OpenGL ES 3.1 AEP

    else()

        message(FATAL_ERROR "Could not determine OpenGL ES version for target platform")

    endif()

endif()

################# Include platform specific CMakeLists ################################

# Note: Make sure that this is after feater enable/disables
#       since link libraries are added based on these flags!

if (MSVC)

    include("${CMAKE_SOURCE_DIR}/CMakeLists_MSVC.cmake")

elseif (ANDROID)

    include("${CMAKE_SOURCE_DIR}/CMakeLists_ANDROID.cmake")

else()

    message(FATAL_ERROR "Could not autodetect target OS.")

endif()

################################################################################

# Capitalize target names
string(SUBSTRING ${OMAF_CMAKE_TARGET_OS} 0 1 FIRST_LETTER)
string(TOUPPER ${FIRST_LETTER} FIRST_LETTER)
string(REGEX REPLACE "^.(.*)" "${FIRST_LETTER}\\1" LIB_TARGET_NAME "${OMAF_CMAKE_TARGET_OS}")

set(API_LIB "${CMAKE_SOURCE_DIR}/Lib/${LIB_TARGET_NAME}")
 
if (TARGET_OS_ANDROID)
    #On android we need to add ABI to the path..
    set(OMAF_EXT_PATH_RELEASE_PREFIX ${OMAF_CMAKE_TARGET_OS}/release/${ANDROID_ABI})
    set(OMAF_EXT_PATH_DEBUG_PREFIX ${OMAF_CMAKE_TARGET_OS}/debug/${ANDROID_ABI})
else()
    set(OMAF_EXT_PATH_RELEASE_PREFIX ${OMAF_CMAKE_TARGET_OS}/release/${VSVERSION})
    set(OMAF_EXT_PATH_DEBUG_PREFIX ${OMAF_CMAKE_TARGET_OS}/debug/${VSVERSION})
endif()


if (TARGET_OS_WINDOWS)
	set(OMAF_COMMON_DEPS ${OMAF_COMMON_DEPS}
		$<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/../Lib/Windows/Debug/${VSVERSION}/dash.${dashsuffix}>
		$<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/../Lib/Windows/Release/${VSVERSION}/dash.${dashsuffix}>
		)

	set(OMAF_COMMON_DEPS ${OMAF_COMMON_DEPS}
		$<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/../Lib/Windows/Debug/${VSVERSION}/heif_shared.lib>
		$<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/../Lib/Windows/Release/${VSVERSION}/heif_shared.lib>
		)
else()
	set(OMAF_COMMON_DEPS ${OMAF_COMMON_DEPS}
		$<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/../Lib/${LIB_TARGET_NAME}/Debug/${ANDROID_ABI}/libdash.${dashsuffix}>
		$<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/../Lib/${LIB_TARGET_NAME}/Release/${ANDROID_ABI}/libdash.${dashsuffix}>
		)

	set(OMAF_COMMON_DEPS ${OMAF_COMMON_DEPS}
		$<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/../Lib/${LIB_TARGET_NAME}/Debug/${ANDROID_ABI}/libheif_shared.${dashsuffix}>
		$<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/../Lib/${LIB_TARGET_NAME}/Release/${ANDROID_ABI}/libheif_shared.${dashsuffix}>
		)
endif()


add_definitions(-DOMAF_ENABLE_STREAM_VIDEO_PROVIDER)

if (ENABLE_MEMORY_TRACKING)
    add_definitions(-DOMAF_MEMORY_TRACKING=1)
endif()

add_subdirectory(Sources)

#enable logging in all builds
target_compile_definitions(OMAFPlayer PUBLIC OMAF_ENABLE_LOG)

target_compile_definitions(OMAFPlayer PUBLIC $<$<CONFIG:Debug>:DEBUG>)

if (TARGET_OS_WINDOWS)

	# win32_x64
	# win32_x86

	set(SDK_INSTALL_SCRIPT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/CopyBins.bat")

	file(WRITE "${SDK_INSTALL_SCRIPT}" "@echo off\n")
	file(APPEND "${SDK_INSTALL_SCRIPT}" "echo Copying files for %*\n")
	if (CMAKE_GENERATOR STREQUAL "Ninja")
		if (CMAKE_BUILD_TYPE STREQUAL "Debug")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "\"${CMAKE_COMMAND}\" -E make_directory \"${API_LIB}/Debug/win32_x64/\"\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "\"${CMAKE_COMMAND}\" -E copy \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/OMAFPlayerd.dll\" \"${API_LIB}/Debug/win32_x64/\"\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "\"${CMAKE_COMMAND}\" -E copy \"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/OMAFPlayerd.lib\" \"${API_LIB}/Debug/win32_x64/\"\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "\"${CMAKE_COMMAND}\" -E copy \"${CMAKE_PDB_OUTPUT_DIRECTORY}/OMAFPlayerd.pdb\" \"${API_LIB}/Debug/win32_x64/\"\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "\"${CMAKE_COMMAND}\" -E copy \"${CMAKE_SOURCE_DIR}/../Mp4/lib/Windows/Debug/mp4vr_shared.dll\" \"${API_LIB}/Debug/win32_x64/\"\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "\"${CMAKE_COMMAND}\" -E copy \"${CMAKE_SOURCE_DIR}/../Lib/Windows/Debug/${VSVERSION}/dash.dll\" \"${API_LIB}/Debug/win32_x64/\"\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "\"${CMAKE_COMMAND}\" -E copy \"${CMAKE_SOURCE_DIR}/../Lib/Windows/Debug/${VSVERSION}/heif_shared.dll\" \"${API_LIB}/Debug/win32_x64/\"\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		else()
			file(APPEND "${SDK_INSTALL_SCRIPT}" "\"${CMAKE_COMMAND}\" -E make_directory \"${API_LIB}/Release/win32_x64/\"\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "\"${CMAKE_COMMAND}\" -E copy \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/OMAFPlayer.dll\" \"${API_LIB}/Release/win32_x64/\"\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "\"${CMAKE_COMMAND}\" -E copy \"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/OMAFPlayer.lib\" \"${API_LIB}/Release/win32_x64/\"\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "\"${CMAKE_COMMAND}\" -E copy \"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/OMAFPlayer.pdb\" \"${API_LIB}/Release/win32_x64/\"\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "\"${CMAKE_COMMAND}\" -E copy \"${CMAKE_SOURCE_DIR}/../Mp4/lib/Windows/Release/mp4vr_shared.dll\" \"${API_LIB}/Release/win32_x64/\"\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "\"${CMAKE_COMMAND}\" -E copy \"${CMAKE_SOURCE_DIR}/../Lib/Windows/Release/${VSVERSION}/dash.dll\" \"${API_LIB}/Release/win32_x64/\"\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "\"${CMAKE_COMMAND}\" -E copy \"${CMAKE_SOURCE_DIR}/../Lib/Windows/Release/${VSVERSION}/heif_shared.dll\" \"${API_LIB}/Release/win32_x64/\"\n")
			file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		endif()
	else()
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if \"%1\"==\"Debug\" \"${CMAKE_COMMAND}\" -E make_directory \"${API_LIB}/Debug/win32_x64/\"\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if \"%1\"==\"Debug\" \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/debug/OMAFPlayerd.dll\" \"${API_LIB}/Debug/win32_x64/\"\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if \"%1\"==\"Debug\" \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/debug/OMAFPlayerd.lib\" \"${API_LIB}/Debug/win32_x64/\"\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if \"%1\"==\"Debug\" \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_PDB_OUTPUT_DIRECTORY}/debug/OMAFPlayerd.pdb\" \"${API_LIB}/Debug/win32_x64/\"\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if \"%1\"==\"Debug\" \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_SOURCE_DIR}/../Mp4/lib/Windows/Debug/mp4vr_shared.dll\" \"${API_LIB}/Debug/win32_x64/\"\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if \"%1\"==\"Debug\" \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_SOURCE_DIR}/../Lib/Windows/Debug/${VSVERSION}/dash.dll\" \"${API_LIB}/Debug/win32_x64/\"\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if \"%1\"==\"Debug\" \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_SOURCE_DIR}/../Lib/Windows/Debug/${VSVERSION}/heif_shared.dll\" \"${API_LIB}/Debug/win32_x64/\"\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if \"%1\"==\"Release\" \"${CMAKE_COMMAND}\" -E make_directory \"${API_LIB}/Release/win32_x64/\"\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if \"%1\"==\"Release\" \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/release/OMAFPlayer.dll\" \"${API_LIB}/Release/win32_x64/\"\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if \"%1\"==\"Release\" \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/release/OMAFPlayer.lib\" \"${API_LIB}/Release/win32_x64/\"\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if \"%1\"==\"Release\" \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/release/OMAFPlayer.pdb\" \"${API_LIB}/Release/win32_x64/\"\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if \"%1\"==\"Release\" \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_SOURCE_DIR}/../Mp4/lib/Windows/Release/mp4vr_shared.dll\" \"${API_LIB}/Release/win32_x64/\"\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if \"%1\"==\"Release\" \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_SOURCE_DIR}/../Lib/Windows/Release/${VSVERSION}/dash.dll\" \"${API_LIB}/Release/win32_x64/\"\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if \"%1\"==\"Release\" \"${CMAKE_COMMAND}\" -E copy \"${CMAKE_SOURCE_DIR}/../Lib/Windows/Release/${VSVERSION}/heif_shared.dll\" \"${API_LIB}/Release/win32_x64/\"\n")
		file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :cmEnd\n")
	endif()
	file(APPEND "${SDK_INSTALL_SCRIPT}" ":cmEnd\n")
	file(APPEND "${SDK_INSTALL_SCRIPT}" "endlocal & call :cmErrorLevel %errorlevel% & goto :cmDone\n")
	file(APPEND "${SDK_INSTALL_SCRIPT}" ":cmErrorLevel\n")
	file(APPEND "${SDK_INSTALL_SCRIPT}" "exit /b %1\n")
	file(APPEND "${SDK_INSTALL_SCRIPT}" ":cmDone\n")
	file(APPEND "${SDK_INSTALL_SCRIPT}" "if %errorlevel% neq 0 goto :VCEnd\n")
	file(APPEND "${SDK_INSTALL_SCRIPT}" ":VCEnd\n")	
	
elseif (TARGET_OS_ANDROID)

	# arm64-v8a
	# armeabi
	# armeabi-v7a
	# x86

	set(DEBUG_API_FILES $<TARGET_FILE:OMAFPlayer>
		"${CMAKE_SOURCE_DIR}/../Mp4/lib/android/debug/${ANDROID_ABI}/libmp4vr_shared.so"
		"${CMAKE_SOURCE_DIR}/../Lib/Android/Debug/${ANDROID_ABI}/libdash.so"
		"${CMAKE_SOURCE_DIR}/../Lib/Android/Debug/${ANDROID_ABI}/libheif_shared.so"
	)
	set(RELEASE_API_FILES $<TARGET_FILE:OMAFPlayer>
		"${CMAKE_SOURCE_DIR}/../Mp4/lib/android/release/${ANDROID_ABI}/libmp4vr_shared.so"
		"${CMAKE_SOURCE_DIR}/../Lib/Android/Release/${ANDROID_ABI}/libdash.so"
		"${CMAKE_SOURCE_DIR}/../Lib/Android/Release/${ANDROID_ABI}/libheif_shared.so"
	)

	#using a custom target here.
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		add_custom_target(api_install ALL
			COMMAND ${CMAKE_COMMAND} -E make_directory "${API_LIB}/Debug/${ANDROID_ABI}"
			COMMAND ${CMAKE_COMMAND} -E copy ${DEBUG_API_FILES} "${API_LIB}/Debug/${ANDROID_ABI}/"
			)
	else()
		add_custom_target(api_install ALL
		   COMMAND ${CMAKE_COMMAND} -E make_directory "${API_LIB}/Release/${ANDROID_ABI}"
		   COMMAND ${CMAKE_COMMAND} -E copy ${RELEASE_API_FILES} "${API_LIB}/Release/${ANDROID_ABI}/"
		   COMMAND ${CMAKE_OBJCOPY} --strip-debug "${API_LIB}/Release/${ANDROID_ABI}/$<TARGET_FILE_NAME:OMAFPlayer>"
		   )
	endif()

	add_dependencies(api_install OMAFPlayer)

else()

	message(FATAL_ERROR "api install for target platform not done yet!")

endif()

