subdir('icons')

panel_color_sources = [
  'cc-color-panel.c',
  'color-module.c',
]

panel_color = shared_library('color',
  panel_color_sources,
  include_directories: rootInclude,
  link_with: [
    libcinnamon_control_center,
  ],
  dependencies: [
    gtk,
    colord,
    cinn_desktop,
  ],
  install: true,
  install_dir: panels_dir
)


install_data('color.ui',
  install_dir: ui_dir,
)


i18n.merge_file(
  input : 'cinnamon-color-panel.desktop.in.in',
  output: 'cinnamon-color-panel.desktop',
  po_dir: po_dir,
  type: 'desktop',
  install: true,
  install_dir: get_option('datadir') / 'applications',
)
