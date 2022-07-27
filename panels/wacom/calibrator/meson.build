calibrator_inc = include_directories('.')

common_sources = files(
  'calibrator.c',
  'calibrator-gui.c',
  'cc-clock.c',
)

calibrator_deps = deps + [math]

libwacom_calibrator = static_library(
  'wacom-calibrator',
  sources: common_sources,
  include_directories: rootInclude,
  dependencies: calibrator_deps,
  c_args: more_cflags
)

libwacom_calibrator_test = static_library(
  'wacom-calibrator-test',
  sources: common_sources,
  include_directories: rootInclude,
  dependencies: calibrator_deps,
  c_args: test_cflags
)

sources = common_sources + wacom_gresource + files('main.c')

executable(
  'test-calibrator',
  sources,
  include_directories: rootInclude,
  dependencies: calibrator_deps,
  c_args: more_cflags
)
