find_package(GUILE)

if(GUILE_FOUND)
  set(
    guilecad_headers
    guilecad.h
  )
  brlcad_manage_files(guilecad_headers ${INCLUDE_DIR}/brlcad/guilecad)
endif(GUILE_FOUND)

cmakefiles(CMakeLists.txt)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
