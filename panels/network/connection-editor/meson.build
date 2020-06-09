
libconnection_editor_sources = [
  'ce-page-8021x-security.c',
  'ce-page-details.c',
  'ce-page-ethernet.c',
  'ce-page-ip4.c',
  'ce-page-ip6.c',
  'ce-page-reset.c',
  'ce-page-security.c',
  'ce-page-vpn.c',
  'ce-page-wifi.c',
  'ce-page.c',
  'firewall-helpers.c',
  'net-connection-editor.c',
  'ui-helpers.c',
  'vpn-helpers.c',
  gnome.compile_resources('net-connection-editor-resources',
    'connection-editor.gresource.xml',
    c_name: 'net_connection_editor',
    source_dir: meson.current_source_dir(),
  ),
]

libconnection_editor = static_library('connection_editor',
  libconnection_editor_sources,
  include_directories: [ rootInclude, wireless_securityInclude ],
  link_whole: [
    libwireless_security,
  ],
  dependencies: [
    gtk,
    libnm,
  ],
)
