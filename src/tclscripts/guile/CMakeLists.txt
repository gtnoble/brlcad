# Install Guile Scheme API files

find_package(GUILE)

if(GUILE_FOUND)
  set(
    guile_SCRIPTS
    brlcad-api.scm
  )
  
  brlcad_adddata(guile_SCRIPTS tclscripts/guile)
endif(GUILE_FOUND)

cmakefiles(
  CMakeLists.txt
  brlcad-api.scm
)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
