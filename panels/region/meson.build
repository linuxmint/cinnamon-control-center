
panel_region_sources = [
  'cc-region-panel.c',
  'cinnamon-region-panel-xkb.c',
  'cinnamon-region-panel-xkblt.c',
  'cinnamon-region-panel-xkbltadd.c',
  'cinnamon-region-panel-xkbot.c',
  'cinnamon-region-panel-xkbpv.c',
  'region-module.c',
]

panel_region = shared_library('region',
  panel_region_sources,
  include_directories: rootInclude,
  link_with: [
    libcinnamon_control_center,
  ],
  dependencies: [
    glib,
    gtk,
    cinn_desktop,
    libgnomekbd,
    libgnomekbdui,
    libxklavier
  ],

  install: true,
  install_dir: panels_dir
)


install_data([
    'cinnamon-region-panel-layout-chooser.ui',
    'cinnamon-region-panel-options-dialog.ui',
    'cinnamon-region-panel.ui',
  ],
  install_dir: ui_dir,
)

# Region panel is imported as a page of cs_keyboard so no share/applications entry needed.
install_data(
  'cinnamon-region-panel.desktop',
  install_dir: panel_def_dir
)