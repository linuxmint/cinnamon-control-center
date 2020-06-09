subdir('wireless-security')
subdir('connection-editor')

panel_network_sources = [
  'cc-network-panel.c',
  'net-device-ethernet.c',
  'net-device-simple.c',
  'net-device-wifi.c',
  'net-device.c',
  'net-object.c',
  'net-proxy.c',
  'net-vpn.c',
  'network-dialogs.c',
  'network-module.c',
  'panel-common.c',
  gnome.compile_resources('cc-network-resources',
    'network.gresource.xml',
    c_name: 'cc_network',
    source_dir: meson.current_source_dir(),
  ),
]

if modemmanager.found()
  panel_network_sources += 'net-device-mobile.c'
endif


panel_network = shared_library('network',
  panel_network_sources,
  include_directories: rootInclude,
  link_whole: [
    libconnection_editor,
  ],
  link_with: [
    libcinnamon_control_center,
  ],
  dependencies: [
    glib,
    gtk,
    libnm,
    libnma,
    modemmanager,
    polkit_gobj,
  ],

  install: true,
  install_dir: panels_dir
)


install_data('network.ui',
  install_dir: ui_dir,
)


i18n.merge_file(
  input : 'cinnamon-network-panel.desktop.in.in',
  output: 'cinnamon-network-panel.desktop',
  po_dir: po_dir,
  type: 'desktop',
  install: true,
  install_dir: get_option('datadir') / 'applications',
)
