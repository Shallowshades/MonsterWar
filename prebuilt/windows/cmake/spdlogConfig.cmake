# Custom prebuilt config for spdlog (static)
find_package(Threads REQUIRED)
if(NOT TARGET spdlog::spdlog)
  add_library(spdlog::spdlog STATIC IMPORTED)
  set_target_properties(spdlog::spdlog PROPERTIES
    IMPORTED_CONFIGURATIONS "Debug;Release"
    IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_LIST_DIR}/../lib/spdlogd.lib"
    IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_LIST_DIR}/../lib/spdlog.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../include"
    INTERFACE_COMPILE_DEFINITIONS "SPDLOG_COMPILED_LIB")
endif()
set(spdlog_FOUND TRUE)
