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
    colord
  ],
  install: true,
  install_dir: panels_dir
)


install_data('color.ui',
  install_dir: ui_dir,
)

install_data(
  'cinnamon-color-panel.desktop',
  install_dir: desktop_dir
)
