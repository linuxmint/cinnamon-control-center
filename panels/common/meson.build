
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

common_inc = include_directories('.')
common_sources = []

enums = 'csd-common-enums'
enums_header = files('csd-device-manager.h')

common_sources += gnome.mkenums(
  enums + '.h',
  sources: enums_header,
  fhead: '#pragma once\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n',
  fprod: '/* enumerations from "@filename@" */\n',
  vhead: 'GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define CSD_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n',
  ftail: 'G_END_DECLS\n'
)

common_sources += gnome.mkenums(
  enums + '.c',
  sources: enums_header,
  fhead: '#include "csd-device-manager.h"\n#include "csd-common-enums.h"\n',
  fprod: '\n/* enumerations from "@filename@" */',
  vhead: 'GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {',
  vprod: '      { @VALUENAME@, "@VALUENAME@", "@valuenick@" },',
  vtail: '      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static ("@EnumName@", values);\n  }\n  return etype;\n}\n'
)

csd_headers = [
  'csd-device-manager.h',
  'csd-input-helper.h',
]

csd_sources = [
  'csd-device-manager.c',
  'csd-input-helper.c'
]

sources = common_sources + files(csd_sources)

deps = [ gudev_dep, glib, gio_unix, gtk, cinn_desktop ]

libdevice = static_library(
  'device',
  sources: sources,
  include_directories: rootInclude,
  dependencies: deps
)

libdevice_dep = declare_dependency(
  include_directories: common_inc,
  link_with: libdevice
)
