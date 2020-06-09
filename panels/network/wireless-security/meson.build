wireless_securityInclude = include_directories('.')

libwireless_security_sources = [
  'eap-method-fast.c',
  'eap-method-leap.c',
  'eap-method-peap.c',
  'eap-method-simple.c',
  'eap-method-tls.c',
  'eap-method-ttls.c',
  'eap-method.c',
  'helpers.c',
  'utils.c',
  'wireless-security.c',
  'ws-dynamic-wep.c',
  'ws-leap.c',
  'ws-wep-key.c',
  'ws-wpa-eap.c',
  'ws-wpa-psk.c',
  gnome.compile_resources('wireless-security-resources',
    'wireless-security.gresource.xml',
    c_name: 'wireless_security',
    source_dir: meson.current_source_dir(),
  ),
]

libwireless_security = static_library('wireless_security',
  libwireless_security_sources,
  include_directories: rootInclude,
  dependencies: [
    gtk,
    libnm,
    libnma,
  ],
)

run_target('update-from-nma', command: find_program('update_from_nma.sh'))
