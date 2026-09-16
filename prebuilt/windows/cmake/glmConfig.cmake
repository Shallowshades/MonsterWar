# Custom prebuilt config for glm (header-only)
if(NOT TARGET glm::glm)
  add_library(glm::glm INTERFACE IMPORTED)
  set_target_properties(glm::glm PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../include")
endif()
set(glm_FOUND TRUE)
