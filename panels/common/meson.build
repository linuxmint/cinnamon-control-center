
panel_languages = static_library('language',
  'cc-common-language.c',
  'cc-language-chooser.c',
  'gdm-languages.c',
  include_directories: rootInclude,
  dependencies: [
    glib,
    gtk,
  ],
)

list_languages = executable('list-languages',
  'list-languages.c',
  include_directories: rootInclude,
  link_with: panel_languages,
  dependencies: [
    glib,
  ],
)

install_data('language-chooser.ui',
  install_dir: ui_dir,
)
