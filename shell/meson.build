
##############################################################################
# libcinnamon-control-center

libcinnamon_control_center_sources = [
  'cc-editable-entry.c',
  'cc-panel.c',
  'cc-shell.c',
  'hostname-helper.c',
  'list-box-helper.c',
]
libcinnamon_control_center_headers = [
  'cc-panel.h',
  'cc-shell.h',
  'cc-editable-entry.h',
]

libcinnamon_control_center = shared_library('cinnamon-control-center',
  libcinnamon_control_center_sources,
  include_directories: rootInclude,
  dependencies: [
    gtk,
  ],
  soversion: lib_ver,
  install: true,
)

install_headers(libcinnamon_control_center_headers,
  subdir: 'cinnamon-control-center-1/libcinnamon-control-center',
)

##############################################################################
# cinnamon-control-center

controlcenter_sources = [
  'cc-shell-category-view.c',
  'cc-shell-item-view.c',
  'cc-shell-log.c',
  'cc-shell-model.c',
  'cc-shell-nav-bar.c',
  'cinnamon-control-center.c',
  'control-center.c',
  gnome.genmarshal('cc-shell-marshal',
    prefix: 'cc_shell_marshal',
    sources: 'cc-shell-marshal.list',
  ),
]

controlcenter = executable('cinnamon-control-center',
  controlcenter_sources,

  include_directories: rootInclude,
  dependencies: [
    gtk,
    cinn_menu,
    libx11,
    libnotify,
  ],
  link_with: libcinnamon_control_center,
)

##############################################################################
# Data

pkgconfig.generate(
  name: 'libcinnamon-control-center',
  description: 'A library to create Cinnamon Control Center extensions',
  version: meson.project_version(),
  requires: [
    'gio-2.0',
    'glib-2.0',
    'gtk+-3.0',
  ],
  libraries: [
    libcinnamon_control_center,
  ],
  subdirs: [
    'cinnamon-control-center-1',
  ],
  variables: [
    'extensiondir=${libdir}/cinnamon-control-center-1/panels',
  ],
)


dataconf = configuration_data()
dataconf.set('applicationsdir',
  join_paths(get_option('prefix'), get_option('datadir'), 'applications')
)
install_data(
  configure_file(
    input : 'cinnamoncc.menu.in',
    output: 'cinnamoncc.menu',
    configuration: dataconf,
  ),
  install_dir: join_paths('/', menu_dir),
)


i18n.merge_file(
  input : 'cinnamon-control-center.desktop.in',
  output: 'cinnamon-control-center.desktop',
  po_dir: po_dir,
  type: 'desktop',
  install_dir: join_paths(get_option('datadir'), 'applications'),
)

i18n.merge_file(
  input : 'cinnamoncc.directory.in',
  output: 'cinnamoncc.directory',
  po_dir: po_dir,
  type: 'desktop',
  install_dir: join_paths(get_option('datadir'), 'desktop-directories'),
)

install_data('shell.ui',
  install_dir: ui_dir,
)
