# Custom prebuilt config for EnTT (header-only)
if(NOT TARGET EnTT::EnTT)
  add_library(EnTT::EnTT INTERFACE IMPORTED)
  set_target_properties(EnTT::EnTT PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../include")
endif()
set(EnTT_FOUND TRUE)
