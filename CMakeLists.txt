cmake_minimum_required (VERSION 3.10)
project (LUCC CXX)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules" ${CMAKE_MODULE_PATH})

# When doing a top level build we don't want to do find_package
if (NOT TARGET Unr::Unr)
	find_package(Unr REQUIRED)
endif()

if ( APPLE )
	add_definitions("-D_XOPEN_SOURCE=600")
endif()

set(LUCC_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(lucc
	${LUCC_ROOT}/ClassExport.cpp
	${LUCC_ROOT}/FullPkgExport.cpp
	${LUCC_ROOT}/LevelExport.cpp
	${LUCC_ROOT}/LevelViewer.cpp
	${LUCC_ROOT}/lucc.cpp
	${LUCC_ROOT}/MeshExport.cpp
	${LUCC_ROOT}/MissingNativeFields.cpp
	${LUCC_ROOT}/MusicExport.cpp
	${LUCC_ROOT}/ObjectExport.cpp
	${LUCC_ROOT}/PlayMusic.cpp
	${LUCC_ROOT}/SoundExport.cpp
	${LUCC_ROOT}/TextureExport.cpp
)

target_include_directories(lucc
	PRIVATE
		${LUCC_ROOT}
)

target_link_libraries(lucc
	PRIVATE
		Unr::Unr
)
