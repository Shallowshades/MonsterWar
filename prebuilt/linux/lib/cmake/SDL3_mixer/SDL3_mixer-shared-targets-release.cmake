#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SDL3_mixer::SDL3_mixer-shared" for configuration "Release"
set_property(TARGET SDL3_mixer::SDL3_mixer-shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL3_mixer::SDL3_mixer-shared PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "SDL3::SDL3-shared"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libSDL3_mixer.so.0.2.0"
  IMPORTED_SONAME_RELEASE "libSDL3_mixer.so.0"
  )

list(APPEND _cmake_import_check_targets SDL3_mixer::SDL3_mixer-shared )
list(APPEND _cmake_import_check_files_for_SDL3_mixer::SDL3_mixer-shared "${_IMPORT_PREFIX}/lib/libSDL3_mixer.so.0.2.0" )

# Import target "SDL3_mixer::ogg" for configuration "Release"
set_property(TARGET SDL3_mixer::ogg APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL3_mixer::ogg PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libogg.so.0.8.5"
  IMPORTED_SONAME_RELEASE "libogg.so.0"
  )

list(APPEND _cmake_import_check_targets SDL3_mixer::ogg )
list(APPEND _cmake_import_check_files_for_SDL3_mixer::ogg "${_IMPORT_PREFIX}/lib/libogg.so.0.8.5" )

# Import target "SDL3_mixer::opus" for configuration "Release"
set_property(TARGET SDL3_mixer::opus APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL3_mixer::opus PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopus.so.0.9.0"
  IMPORTED_SONAME_RELEASE "libopus.so.0"
  )

list(APPEND _cmake_import_check_targets SDL3_mixer::opus )
list(APPEND _cmake_import_check_files_for_SDL3_mixer::opus "${_IMPORT_PREFIX}/lib/libopus.so.0.9.0" )

# Import target "SDL3_mixer::opusfile" for configuration "Release"
set_property(TARGET SDL3_mixer::opusfile APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL3_mixer::opusfile PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopusfile.so.0.12"
  IMPORTED_SONAME_RELEASE "libopusfile.so.0"
  )

list(APPEND _cmake_import_check_targets SDL3_mixer::opusfile )
list(APPEND _cmake_import_check_files_for_SDL3_mixer::opusfile "${_IMPORT_PREFIX}/lib/libopusfile.so.0.12" )

# Import target "SDL3_mixer::vorbis" for configuration "Release"
set_property(TARGET SDL3_mixer::vorbis APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL3_mixer::vorbis PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libvorbis.so.0.4.9"
  IMPORTED_SONAME_RELEASE "libvorbis.so.0"
  )

list(APPEND _cmake_import_check_targets SDL3_mixer::vorbis )
list(APPEND _cmake_import_check_files_for_SDL3_mixer::vorbis "${_IMPORT_PREFIX}/lib/libvorbis.so.0.4.9" )

# Import target "SDL3_mixer::vorbisfile" for configuration "Release"
set_property(TARGET SDL3_mixer::vorbisfile APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL3_mixer::vorbisfile PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libvorbisfile.so.3.3.8"
  IMPORTED_SONAME_RELEASE "libvorbisfile.so.3"
  )

list(APPEND _cmake_import_check_targets SDL3_mixer::vorbisfile )
list(APPEND _cmake_import_check_files_for_SDL3_mixer::vorbisfile "${_IMPORT_PREFIX}/lib/libvorbisfile.so.3.3.8" )

# Import target "SDL3_mixer::FLAC" for configuration "Release"
set_property(TARGET SDL3_mixer::FLAC APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL3_mixer::FLAC PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE "SDL3_mixer::ogg"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libFLAC.so.8.3.0"
  IMPORTED_SONAME_RELEASE "libFLAC.so.8"
  )

list(APPEND _cmake_import_check_targets SDL3_mixer::FLAC )
list(APPEND _cmake_import_check_files_for_SDL3_mixer::FLAC "${_IMPORT_PREFIX}/lib/libFLAC.so.8.3.0" )

# Import target "SDL3_mixer::gme_shared" for configuration "Release"
set_property(TARGET SDL3_mixer::gme_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL3_mixer::gme_shared PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libgme.so.0.6.4"
  IMPORTED_SONAME_RELEASE "libgme.so.0"
  )

list(APPEND _cmake_import_check_targets SDL3_mixer::gme_shared )
list(APPEND _cmake_import_check_files_for_SDL3_mixer::gme_shared "${_IMPORT_PREFIX}/lib/libgme.so.0.6.4" )

# Import target "SDL3_mixer::xmp_shared" for configuration "Release"
set_property(TARGET SDL3_mixer::xmp_shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL3_mixer::xmp_shared PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libxmp.so.4.7.0"
  IMPORTED_SONAME_RELEASE "libxmp.so.4"
  )

list(APPEND _cmake_import_check_targets SDL3_mixer::xmp_shared )
list(APPEND _cmake_import_check_files_for_SDL3_mixer::xmp_shared "${_IMPORT_PREFIX}/lib/libxmp.so.4.7.0" )

# Import target "SDL3_mixer::libmpg123" for configuration "Release"
set_property(TARGET SDL3_mixer::libmpg123 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL3_mixer::libmpg123 PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libmpg123.so"
  IMPORTED_SONAME_RELEASE "libmpg123.so"
  )

list(APPEND _cmake_import_check_targets SDL3_mixer::libmpg123 )
list(APPEND _cmake_import_check_files_for_SDL3_mixer::libmpg123 "${_IMPORT_PREFIX}/lib/libmpg123.so" )

# Import target "SDL3_mixer::WavPack" for configuration "Release"
set_property(TARGET SDL3_mixer::WavPack APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SDL3_mixer::WavPack PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libwavpack.so.1.2.8"
  IMPORTED_SONAME_RELEASE "libwavpack.so.1"
  )

list(APPEND _cmake_import_check_targets SDL3_mixer::WavPack )
list(APPEND _cmake_import_check_files_for_SDL3_mixer::WavPack "${_IMPORT_PREFIX}/lib/libwavpack.so.1.2.8" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
