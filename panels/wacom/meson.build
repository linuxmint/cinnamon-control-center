subdir('calibrator')

panel_wacom_sources = [
  'cc-wacom-mapping-panel.c',
  'cc-wacom-nav-button.c',
  'cc-wacom-page.c',
  'cc-wacom-panel.c',
  'cc-wacom-stylus-page.c',
  'csd-input-helper.c',
  'csd-wacom-device.c',
  'wacom-module.c',
  gnome.compile_resources('cc-wacom-resources',
    'wacom.gresource.xml',
    c_name: 'cc_wacom',
    source_dir: meson.current_source_dir(),
  ),
]

panel_wacom = shared_library('wacom-properties',
  panel_wacom_sources,
  include_directories: [ rootInclude, include_directories('calibrator') ],
  link_whole: [
    libwacom_calibrator,
  ],
  link_with: [
    libcinnamon_control_center,
  ],
  dependencies: [
    glib,
    gtk,
    cinn_desktop,
    cinn_settings_daemon,
    libwacom,
    libx11,
    libxi,
  ],

  install: true,
  install_dir: panels_dir
)


# install_data('wacom.ui',
#   install_dir: ui_dir,
# )


i18n.merge_file(
  input : 'cinnamon-wacom-panel.desktop.in.in',
  output: 'cinnamon-wacom-panel.desktop',
  po_dir: po_dir,
  type: 'desktop',
  install: true,
  install_dir: get_option('datadir') / 'applications',
)
