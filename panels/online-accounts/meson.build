subdir('icons')

panel_online_accounts_sources = [
  'cc-online-accounts-panel.c',
  'online-accounts-module.c',
  gnome.compile_resources('cc-online-accounts-resources',
    'online-accounts.gresource.xml',
    c_name: 'cc_online_accounts',
    source_dir: meson.current_source_dir(),
  ),
]

panel_online_accounts = shared_library('online-accounts',
  panel_online_accounts_sources,
  include_directories: rootInclude,
  link_with: [
    libcinnamon_control_center,
  ],
  dependencies: [
    glib,
    gtk,
    goa,
    goa_backend,
  ],

  install: true,
  install_dir: panels_dir
)


install_data('online-accounts.ui',
  install_dir: ui_dir,
)
