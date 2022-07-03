links = [
  'cinnamon-display-panel.desktop',
]
if colord.found()
  links += ['cinnamon-color-panel.desktop']
endif
if libnm.found()
  links += ['cinnamon-network-panel.desktop']
endif
if libwacom.found()
  links += ['cinnamon-wacom-panel.desktop']
endif

if meson.version().version_compare('>=0.61.0')
  foreach link: links
    install_symlink(link,
      install_dir: panel_def_dir,
      pointing_to: '../../applications' / link
    )
  endforeach
else
  meson.add_install_script('desktop-file-links.py')
endif
