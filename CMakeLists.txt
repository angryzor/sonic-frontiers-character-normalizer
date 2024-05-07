cmake_minimum_required(VERSION 3.28)
project(character-normalizer VERSION 0.1.2
                 DESCRIPTION "Character Normalizer"
                 LANGUAGES CXX)

set(PROJECT_AUTHOR "angryzor")
set(PROJECT_LONG_DESCRIPTION "Fills in missing player_common parameters for other characters than Sonic.")
set(PROJECT_COPYRIGHT "Copyright (c) 2024 angryzor - Licensed under GPL3")
set(PROJECT_AUTHOR_URL "https://github.com/angryzor")
set(PROJECT_UPDATE_SERVER "https://raw.githubusercontent.com/angryzor/hmm-update-server/main/sonic-frontiers/character-normalizer/")
string(TIMESTAMP PROJECT_BUILD_DATE "%Y-%m-%d")

set(GAME_FOLDER "C:\\Program Files (x86)\\Steam\\steamapps\\common\\SonicFrontiers" CACHE STRING "The location of your game folder.")
cmake_path(SET GAME_FOLDER_NORMALIZED NORMALIZE ${GAME_FOLDER})
cmake_path(APPEND GAME_FOLDER_NORMALIZED "SonicFrontiers.exe" OUTPUT_VARIABLE SONIC_FRONTIERS_GAME_EXECUTABLE)

if(DEFINED ENV{SONIC_FRONTIERS_SDK})
    message("SONIC_FRONTIERS_SDK environment variable found. Using this version instead of the local submodule.")
    cmake_path(SET RANGERS_SDK_PATH NORMALIZE $ENV{SONIC_FRONTIERS_SDK})
else()
    set(RANGERS_SDK_PATH rangers-sdk)
endif()

add_subdirectory(vendor EXCLUDE_FROM_ALL)
add_subdirectory(${RANGERS_SDK_PATH} rangers-sdk EXCLUDE_FROM_ALL)

add_library(mod SHARED)
target_compile_features(mod PRIVATE cxx_std_17)
target_compile_options(mod PRIVATE $<$<CONFIG:Debug>:/ZI>)
target_compile_definitions(mod
    PRIVATE
        PROJECT_BINARY_FILENAME=$<TARGET_FILE_NAME:mod>
        PROJECT_NAME="${PROJECT_NAME}"
        PROJECT_AUTHOR="${PROJECT_AUTHOR}"
        PROJECT_DESCRIPTION="${PROJECT_DESCRIPTION}"
        PROJECT_LONG_DESCRIPTION="${PROJECT_LONG_DESCRIPTION}"
        PROJECT_COPYRIGHT="${PROJECT_COPYRIGHT}"
        PROJECT_AUTHOR_URL="${PROJECT_AUTHOR_URL}"
        PROJECT_UPDATE_SERVER="${PROJECT_UPDATE_SERVER}"
        PROJECT_BUILD_DATE="${PROJECT_BUILD_DATE}"
        PROJECT_VERSION=${PROJECT_VERSION}
        PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)
set_target_properties(mod
    PROPERTIES
        OUTPUT_NAME ${PROJECT_NAME}
        VS_DEBUGGER_COMMAND ${SONIC_FRONTIERS_GAME_EXECUTABLE}
        VS_DEBUGGER_WORKING_DIRECTORY ${GAME_FOLDER_NORMALIZED}
)

foreach(LIB rangers-sdk detours)
    target_link_libraries(mod PRIVATE ${LIB})
endforeach()

add_subdirectory(src)
configure_file(Mod.ini.in Mod.ini)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT mod)

cmake_path(APPEND GAME_FOLDER_NORMALIZED "Mods/${PROJECT_NAME}" OUTPUT_VARIABLE MOD_FOLDER)

install(TARGETS mod RUNTIME DESTINATION "${MOD_FOLDER}")
install(DIRECTORY resources/ DESTINATION "${MOD_FOLDER}")
install(FILES LICENSE.txt "${CMAKE_CURRENT_BINARY_DIR}/mod.ini" DESTINATION "${MOD_FOLDER}")
