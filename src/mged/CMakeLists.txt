if(BRLCAD_ENABLE_TK)
  set(TCL_ENABLE_TK ON)
endif(BRLCAD_ENABLE_TK)

find_package_tcl()

# ECL support for alternative extension language
if(BRLCAD_ENABLE_ECL)
  find_package(ECL REQUIRED)
  if(ECL_FOUND)
    message("-- Found ECL: ${ECL_LIBRARIES}")
    message("-- ECL include dirs: ${ECL_INCLUDE_DIRS}")
  else(ECL_FOUND)
    message("ECL enablement requested, but ECL is not found - disabling")
    set(BRLCAD_ENABLE_ECL OFF CACHE BOOL "ECL not found" FORCE)
  endif(ECL_FOUND)
endif(BRLCAD_ENABLE_ECL)

set(
  MGED_SOURCES
  adc.c
  arbs.c
  attach.c
  axes.c
  buttons.c
  chgmodel.c
  chgtree.c
  chgview.c
  clone.c
  cmd.c
  color_scheme.c
  columns.c
  dm-generic.c
  dodraw.c
  doevent.c
  dozoom.c
  edarb.c
  edars.c
  edpipe.c
  edsol.c
  f_db.c
  facedef.c
  fbserv.c
  grid.c
  history.c
  mater.c
  menu.c
  mged.c
  mover.c
  overlay.c
  plot.c
  predictor.c
  rect.c
  rtif.c
  scroll.c
  set.c
  setup.c
  share.c
  tedit.c
  titles.c
  track.c
  update.c
  usepen.c
  utility1.c
  vparse.c
  wdb_obj.c
)

# Add ECL source files if ECL is enabled
if(BRLCAD_ENABLE_ECL AND ECL_FOUND)
  set(MGED_SOURCES ${MGED_SOURCES} ecl_interface.c ecl_cmds.c)
  
  # Compile mged_init.lisp to static library using ECL
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libmged_init.a
    COMMAND ${ECL_EXECUTABLE} -load ${CMAKE_CURRENT_SOURCE_DIR}/compile_lisp.lisp
            -- ${CMAKE_CURRENT_SOURCE_DIR}/mged_init.lisp
            ${CMAKE_CURRENT_BINARY_DIR}/libmged_init.a
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mged_init.lisp
            ${CMAKE_CURRENT_SOURCE_DIR}/compile_lisp.lisp
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Compiling mged_init.lisp with ECL to static library"
  )
  
  # Compile mged_commands.lisp to static library using ECL
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libmged_commands.a
    COMMAND ${ECL_EXECUTABLE} -load ${CMAKE_CURRENT_SOURCE_DIR}/compile_lisp.lisp
            -- ${CMAKE_CURRENT_SOURCE_DIR}/mged_commands.lisp
            ${CMAKE_CURRENT_BINARY_DIR}/libmged_commands.a
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mged_commands.lisp
            ${CMAKE_CURRENT_SOURCE_DIR}/compile_lisp.lisp
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Compiling mged_commands.lisp with ECL to static library"
  )
  
  # Compile mged_repl.lisp to static library using ECL
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libmged_repl.a
    COMMAND ${ECL_EXECUTABLE} -load ${CMAKE_CURRENT_SOURCE_DIR}/compile_lisp.lisp
            -- ${CMAKE_CURRENT_SOURCE_DIR}/mged_repl.lisp
            ${CMAKE_CURRENT_BINARY_DIR}/libmged_repl.a
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mged_repl.lisp
            ${CMAKE_CURRENT_SOURCE_DIR}/compile_lisp.lisp
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Compiling mged_repl.lisp with ECL to static library"
  )
  
  # Compile mged_api.lisp to static library using ECL
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/libmged_api.a
    COMMAND ${ECL_EXECUTABLE} -load ${CMAKE_CURRENT_SOURCE_DIR}/compile_lisp.lisp
            -- ${CMAKE_CURRENT_SOURCE_DIR}/mged_api.lisp
            ${CMAKE_CURRENT_BINARY_DIR}/libmged_api.a
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mged_api.lisp
            ${CMAKE_CURRENT_SOURCE_DIR}/compile_lisp.lisp
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Compiling mged_api.lisp with ECL to static library"
  )
  
  # Create imported library targets for the compiled Lisp code
  add_library(mged_init_lib STATIC IMPORTED GLOBAL)
  set_target_properties(mged_init_lib PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libmged_init.a)
  
  add_library(mged_commands_lib STATIC IMPORTED GLOBAL)
  set_target_properties(mged_commands_lib PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libmged_commands.a)
  
  add_library(mged_repl_lib STATIC IMPORTED GLOBAL)
  set_target_properties(mged_repl_lib PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libmged_repl.a)
  
  add_library(mged_api_lib STATIC IMPORTED GLOBAL)
  set_target_properties(mged_api_lib PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libmged_api.a)
  
  # Create custom targets to ensure the libraries are built
  add_custom_target(mged_init_build
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libmged_init.a)
  add_dependencies(mged_init_lib mged_init_build)
  
  add_custom_target(mged_commands_build
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libmged_commands.a)
  add_dependencies(mged_commands_lib mged_commands_build)
  
  add_custom_target(mged_repl_build
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libmged_repl.a)
  add_dependencies(mged_repl_lib mged_repl_build)
  
  add_custom_target(mged_api_build
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libmged_api.a)
  add_dependencies(mged_api_lib mged_api_build)
endif(BRLCAD_ENABLE_ECL AND ECL_FOUND)

if(HAVE_WINDOWS_H)
  # To associate an icon with the application for Windows (needed
  # for the WiX installer) we must use an rc file.
  enable_language(RC)
  set(MGED_SOURCES ${MGED_SOURCES} mged.rc)
endif(HAVE_WINDOWS_H)

set(
  mged_libs
  libtclcad
  libged
  libwdb
  libdm
  libnmg
  ${WS2_32_LIBRARY}
  ${OPENGL32_LIBRARY}
)

# Ideally we'd build as GUI for Windows to avoid the command window popping up,
# but I'm not sure if that would be a problem for scripting with mged.exe on
# Windows so leave it for now...
if(BRLCAD_ENABLE_TCL)
  set(MGED_INCLUDE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../libtermio
  )
  brlcad_addexec(mged "${MGED_SOURCES}" "${mged_libs}")
  if(TK_LIBRARY)
    target_compile_definitions(mged PRIVATE HAVE_TK=1)
  endif(TK_LIBRARY)
  target_include_directories(mged BEFORE PRIVATE ${MGED_INCLUDE_DIRS})
  
  # Add ECL support if enabled
  if(BRLCAD_ENABLE_ECL AND ECL_FOUND)
    target_compile_definitions(mged PRIVATE HAVE_ECL=1)
    target_include_directories(mged PRIVATE ${ECL_INCLUDE_DIRS})
    target_link_libraries(mged mged_init_lib mged_commands_lib mged_repl_lib mged_api_lib ${ECL_LIBRARIES})
    add_dependencies(mged mged_init_build mged_commands_build mged_repl_build mged_api_build)
  endif(BRLCAD_ENABLE_ECL AND ECL_FOUND)
  
  add_target_deps(mged Tkhtml Tktable itcl_pkgIndex itk_pkgIndex TclIndexBld dm_plugins)

  # MGED needs the tclIndex and pkgIndex files set up correctly
  add_dependencies(mged TclIndexBld)

  # MGED is one of the programs that gets a start menu entry
  set_property(
    INSTALL "${BIN_DIR}/$<TARGET_FILE_NAME:mged>"
    PROPERTY CPACK_START_MENU_SHORTCUTS "MGED ${BRLCAD_VERSION}"
  )
  set_property(INSTALL "${BIN_DIR}/$<TARGET_FILE_NAME:mged>" PROPERTY CPACK_DESKTOP_SHORTCUTS "MGED ${BRLCAD_VERSION}")
else(BRLCAD_ENABLE_TCL)
  cmakefiles(${MGED_SOURCES})
endif(BRLCAD_ENABLE_TCL)

brlcad_addexec(cad_boundp cad_boundp.c "libbu;${M_LIBRARY}")

brlcad_addexec(cad_parea cad_parea.c libbu)

set(
  mged_ignore_files
  CMakeLists.txt
  cmd.h
  f_cmd.h
  menu.h
  mged.h
  mged.ico
  mged.rc
  mged_dm.h
  mged_wdb.h
  sedit.h
)
cmakefiles(${mged_ignore_files})

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
