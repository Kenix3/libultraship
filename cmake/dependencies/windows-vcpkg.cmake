if (USE_AUTO_VCPKG)
	include(cmake/automate-vcpkg.cmake)

	if ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
		set(VCPKG_TRIPLET x86-windows-static)
		set(VCPKG_TARGET_TRIPLET x86-windows-static)
	elseif ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
		set(VCPKG_TRIPLET x64-windows-static)
		set(VCPKG_TARGET_TRIPLET x64-windows-static)
	endif()

	vcpkg_bootstrap()
	vcpkg_install_packages(zlib bzip2 sdl2 glew libzip nlohmann-json tinyxml2 spdlog)    
endif()
