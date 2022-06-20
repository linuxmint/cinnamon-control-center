sources = files(
  'cc-display-panel.c',
  'cc-display-arrangement.c',
  'cc-display-config.c',
  'cc-display-config-dbus.c',
  'cc-display-config-manager-dbus.c',
  'cc-display-config-manager.c',
  'cc-display-settings.c',
  'cc-display-labeler.c',
  'display-module.c'
)

resource_data = files(
  'cc-display-panel.ui',
  'cc-display-settings.ui'
)

sources += gnome.compile_resources(
  'cc-display-resources',
  'display.gresource.xml',
  c_name: 'cc_display',
  dependencies: resource_data,
  export: true
)

deps = [
  colord,
  cinn_desktop,
  math,
  upower_glib_dep,
  cinn_desktop,
  libx11,
  glib
]

install_data('org.cinnamon.control-center.display.gschema.xml',
    install_dir: join_paths(get_option('datadir'), 'glib-2.0', 'schemas')
)

panel_display = shared_library(
  'display',
  link_with: libcinnamon_control_center,
  sources: sources,
  include_directories: [ rootInclude ],
  dependencies: deps,
  install: true,
  install_dir: panels_dir
)

install_data(
  'cinnamon-display-panel.desktop',
  install_dir: desktop_dir
)

subdir('icons')
