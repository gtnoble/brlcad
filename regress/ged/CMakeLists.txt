# Tests for various GED commands

if(TARGET libged)
  brlcad_addexec(regress_mater mater.c libged TEST_USESDATA)
  brlcad_regression_test(regress-ged_mater "regress_mater;ged-mater;density_DATAFILES_cp" EXEC regress_mater)
  distclean("${CMAKE_CURRENT_BINARY_DIR}/regress-ged_mater.log")
  distclean("${CMAKE_CURRENT_BINARY_DIR}/ged_mater_test.g")

  brlcad_addexec(regress_plot plot.c libged TEST_USESDATA)
  brlcad_regression_test(regress-ged_plot "regress_plot" EXEC regress_plot)
  distclean("${CMAKE_CURRENT_BINARY_DIR}/regress-ged_plot.log")
  distclean("${CMAKE_CURRENT_BINARY_DIR}/ged_plot_test.g")

  brlcad_addexec(regress_push push.cpp libged TEST_USESDATA)
  brlcad_regression_test(regress-ged_push "regress_push;ged-npush" EXEC regress_push)
  distclean("${CMAKE_CURRENT_BINARY_DIR}/regress-ged_push.log")
  distclean("${CMAKE_CURRENT_BINARY_DIR}/ged_push.g")

  # MGED Socket Server Tests
  brlcad_addexec(regress_mged_server_protocol "mged_server_protocol.c;${CMAKE_SOURCE_DIR}/src/mged/mged_protocol.c" libged TEST_USESDATA)
  target_include_directories(regress_mged_server_protocol PRIVATE ${CMAKE_SOURCE_DIR}/src/mged)
  brlcad_regression_test(regress-ged_mged_server_protocol "regress_mged_server_protocol" EXEC regress_mged_server_protocol)
  distclean("${CMAKE_CURRENT_BINARY_DIR}/regress-ged_mged_server_protocol.log")

  brlcad_addexec(regress_mged_server_core "mged_server_core.c;${CMAKE_SOURCE_DIR}/src/mged/mged_server.c;${CMAKE_SOURCE_DIR}/src/mged/mged_client.c;${CMAKE_SOURCE_DIR}/src/mged/mged_protocol.c" libged TEST_USESDATA)
  target_include_directories(regress_mged_server_core PRIVATE ${CMAKE_SOURCE_DIR}/src/mged)
  brlcad_regression_test(regress-ged_mged_server_core "regress_mged_server_core" EXEC regress_mged_server_core)
  distclean("${CMAKE_CURRENT_BINARY_DIR}/regress-ged_mged_server_core.log")

  brlcad_addexec(regress_mged_client_tests "mged_client_tests.c;${CMAKE_SOURCE_DIR}/src/mged/mged_client.c;${CMAKE_SOURCE_DIR}/src/mged/mged_protocol.c" libged TEST_USESDATA)
  target_include_directories(regress_mged_client_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/mged)
  brlcad_regression_test(regress-ged_mged_client_tests "regress_mged_client_tests" EXEC regress_mged_client_tests)
  distclean("${CMAKE_CURRENT_BINARY_DIR}/regress-ged_mged_client_tests.log")

  # MGED Socket Server Integration Tests
  brlcad_addexec(regress_mged_integration_tests "mged_integration_tests.c;mged_mock_client.c;${CMAKE_SOURCE_DIR}/src/mged/mged_protocol.c" libged TEST_USESDATA)
  target_include_directories(regress_mged_integration_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/mged)
  brlcad_regression_test(regress-ged_mged_integration_tests "regress_mged_integration_tests" EXEC regress_mged_integration_tests)
  distclean("${CMAKE_CURRENT_BINARY_DIR}/regress-ged_mged_integration_tests.log")
endif(TARGET libged)

cmakefiles(
  CMakeLists.txt
  mater.c
  plot.c
  push.cpp
  ppush_tests.g
  push_tests.g
  regress-ged_mater.cmake.in
  regress-ged_plot.cmake.in
  regress-ged_push.cmake.in
  xpush_tests.g
  mged_server_protocol.c
  mged_server_core.c
  mged_client_tests.c
  mged_mock_client.c
  mged_integration_tests.c
)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
