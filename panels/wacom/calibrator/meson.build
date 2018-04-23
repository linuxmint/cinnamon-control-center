libwacom_calibrator_sources = [
  'calibrator.c',
  'gui_gtk.c',
]

libwacom_calibrator = static_library('wacom-calibrator',
  libwacom_calibrator_sources,
  include_directories: rootInclude,
  dependencies: [
    gtk,
  ],
)

test('wacom-calibrator',
  executable('test-wacom-calibrator',
    'main.c',
    link_with: libwacom_calibrator,
    include_directories: rootInclude,
    dependencies: [
      gtk,
      libxi,
      libx11,
    ],
  )
)
