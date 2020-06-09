
sizes = [
  '16x16',
  '22x22',
  '24x24',
  '32x32',
  '48x48',
  '64x64',
  '256x256',
  'scalable',
]

foreach size : sizes
  install_subdir(size,
    strip_directory: true,
    install_dir:
      get_option('datadir') / 'icons' / 'hicolor' / size / 'apps',
  )
endforeach
