if meson.version().version_compare('>=0.61.0')
  foreach link: [
    'cinnamon-color-panel.desktop',
    'cinnamon-display-panel.desktop',
    'cinnamon-network-panel.desktop',
    'cinnamon-wacom-panel.desktop',
  ]
    install_symlink(link,
      install_dir: panel_def_dir,
      pointing_to: '../../applications' / link
    )
  endforeach
else
  meson.add_install_script('desktop-file-links.py')
endif
