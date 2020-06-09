
subdir('icons')

subdir('po-timezones')


# Datetime data

install_subdir('data',
  install_dir: get_option('datadir') / meson.project_name() / 'ui' / 'datetime',
  strip_directory: true,
)

install_data('datetime.ui',
  install_dir: get_option('datadir') / meson.project_name() / 'ui' / 'datetime',
)

install_data('backward',
  install_dir: get_option('datadir') / meson.project_name() / 'datetime',
)

i18n.merge_file(
  input : 'org.cinnamon.controlcenter.datetime.policy.in',
  output: 'org.cinnamon.controlcenter.datetime.policy',
  po_dir: po_dir,
  install: true,
  install_dir: get_option('datadir') / 'polkit-1' / 'actions',
)


datetime_codegen = [
  gnome.gdbus_codegen('cc-dtm-proxy',
    'timedated1-interface.xml',
    annotations: [
      'org.freedesktop.timedate1',
      'org.gtk.GDBus.C.Name',
      'Dtm'
    ],
    namespace: 'CC'
  ),

  gnome.gdbus_codegen('cc-csddtm-proxy',
    'csd-dtm-interface.xml',
    annotations: [
      'org.cinnamon.SettingsDaemon.DateTimeMechanism',
      'org.gtk.GDBus.C.Name',
      'Csddtm'
    ],
    namespace: 'CC'
  ),
]

panel_datetime = shared_library('date_time',
  'cc-datetime-panel.c',
  'cc-timezone-map.c',
  'date-endian.c',
  'datetime-module.c',
  'tz.c',
  datetime_codegen,
  include_directories: rootInclude,
  link_with: [
    libcinnamon_control_center,
  ],
  dependencies: [
    glib,
    gtk,
    cinn_desktop,
    polkit_gobj,
    math,
  ],

  install: true,
  install_dir: panels_dir
)

test('timezone-gfx',
  executable('test-timezone-gfx',
    'test-timezone-gfx.c',
    'cc-timezone-map.c',
    'tz.c',
    include_directories: rootInclude,
    link_with: panel_datetime,
    dependencies: [
      gtk,
      math,
    ],
  )
)
test('endianess',
  executable('test-endianess',
    'test-endianess.c',
    'tz.c',
    include_directories: rootInclude,
    link_with: panel_datetime,
    dependencies: [
      glib,
      math,
    ],
  )
)
test('timezone',
  executable('test-timezone',
    'test-timezone.c',
    'date-endian.c',
    include_directories: rootInclude,
    link_with: panel_datetime,
    dependencies: [
      glib,
      gtk,
    ],
  )
)
