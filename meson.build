project('cinnamon-control-center', 'c',
  version: '5.0.2',
  meson_version: '>=0.49.0',
)

i18n      = import('i18n')
pkgconfig = import('pkgconfig')
gnome     = import('gnome')
cc        = meson.get_compiler('c')
config    = configuration_data()

###############################################################################
# Project conf

lib_ver = '1.0.0'

gettext_package = meson.project_name()

config.set_quoted('PACKAGE',          meson.project_name())
config.set_quoted('VERSION',          meson.project_version())
config.set_quoted('GETTEXT_PACKAGE',  gettext_package)

###############################################################################
# Dependencies

math = cc.find_library('m')

cinn_desktop= dependency('cinnamon-desktop', version: '>=4.6.0')
cinn_menu   = dependency('libcinnamon-menu-3.0')

gtk         = dependency('gtk+-3.0',        version: '>=3.16.0')
glib        = dependency('glib-2.0',        version: '>=2.44.0')
libgnomekbd = dependency('libgnomekbd',     version: '>=3.0.0')
libgnomekbdui=dependency('libgnomekbdui',   version: '>=3.0.0')
libnotify   = dependency('libnotify',       version: '>=0.7.3')
libx11      = dependency('x11')
polkit_gobj = dependency('polkit-gobject-1',version: '>=0.103')
libxklavier = dependency('libxklavier',     version: '>=5.1')

config.set('HAVE_X11_EXTENSIONS_XKB_H', cc.has_header('X11/extensions/XKB.h'))

###############################################################################
# This is a hard-dependency for the region and user-accounts panels

isocodes = dependency('iso-codes')
config.set_quoted('ISO_CODES_PREFIX', isocodes.get_pkgconfig_variable('prefix'))

###############################################################################
# Network Manager stuff

if get_option('networkmanager')
  libnm         = dependency('libnm', version: '>=1.2.0')
  libnma        = dependency('libnma',version: '>=1.2.0')
else
  libnm = dependency('', required: false)
  libnma= dependency('', required: false)
endif

config.set('HAVE_NMA_18', libnm.version().version_compare('>=1.8.0'))

if get_option('modemmanager')
  if not get_option('networkmanager')
    error('*** NetworkManager is required by ModemManager ***')
  endif

  modemmanager = dependency('mm-glib', version: '>=0.7')
else
  modemmanager = dependency('', required: false)
endif

config.set('BUILD_MODEM', modemmanager.found())


###############################################################################
# Color Management
if get_option('color')
  colord = dependency('colord', version: '>=0.1.14')
else
  colord = dependency('', required: false)
endif

###############################################################################
# Wacom Management
if get_option('wacom')
  libwacom  = dependency('libwacom',  version: '>=0.7')
  libxi     = dependency('xi',        version: '>=1.2')
  cinn_settings_daemon = dependency('cinnamon-settings-daemon', version: '>=1.0.0')
else
  libwacom  = dependency('', required: false)
endif

config.set('HAVE_WACOM_3D_STYLUS', libwacom.version().version_compare('>=0.27'))


###############################################################################
# Online Accounts
if get_option('onlineaccounts')
  goa         = dependency('goa-1.0',         version: '>=3.18.0')
  goa_backend = dependency('goa-backend-1.0', version: '>=3.18.0')
else
  goa         = dependency('', required: false)
  goa_backend = dependency('', required: false)
endif

###############################################################################
# Directories

# Relative paths
panels_dir = get_option('libdir') / 'cinnamon-control-center-1' / 'panels'
ui_dir     = get_option('datadir') / meson.project_name() / 'ui'
um_pix_dir = get_option('datadir') / meson.project_name() / 'pixmaps'
menu_dir   = get_option('sysconfdir') / 'xdg' / 'menus'


prefix = get_option('prefix')
config.set_quoted('BINDIR',               prefix / get_option('bindir'))
config.set_quoted('SBINDIR',              prefix / get_option('sbindir'))
config.set_quoted('LIBLOCALEDIR',         prefix / get_option('libdir') / 'locale')
config.set_quoted('LOCALE_DIR',           prefix / get_option('localedir'))
config.set_quoted('CINNAMONLOCALEDIR',    prefix / get_option('localedir'))
config.set_quoted('DATADIR',              prefix / get_option('datadir'))
config.set_quoted('CINNAMONCC_DATA_DIR',  prefix / get_option('datadir'))
config.set_quoted('CINNAMONCC_UI_DIR',    prefix / ui_dir)
config.set_quoted('PANELS_DIR',           prefix / panels_dir)
config.set_quoted('MENUDIR',              '/' / menu_dir)
config.set_quoted('UIDIR',                prefix / ui_dir)
config.set_quoted('UM_PIXMAP_DIR',        prefix / um_pix_dir)

###############################################################################
# Build options


more_cflags = [
  '-Wno-strict-aliasing',
  '-Wno-sign-compare',
]

if get_option('warning_level').to_int() >=2
  more_cflags += [
    '-Wall',
    '-Wclobbered',
    '-Wempty-body',
    '-Wignored-qualifiers',
    '-Wmissing-field-initializers',
    '-Wmissing-parameter-type',
    '-Wold-style-declaration',
    '-Woverride-init',
    '-Wtype-limits',
    '-Wuninitialized',
    '-Wchar-subscripts',
    '-Wmissing-declarations',
    '-Wmissing-prototypes',
    '-Wnested-externs',
    '-Wpointer-arith',
    '-Wcast-align',
    '-Wsign-compare',
  ]
endif

if get_option('buildtype').contains('rel')
  more_cflags += '-Wp,-D_FORTIFY_SOURCE=2'
endif

if not get_option('deprecated_warnings')
  add_global_arguments([
      '-Wno-deprecated-declarations',
      '-Wno-deprecated',
      '-Wno-declaration-after-statement',
      '-DGLIB_DISABLE_DEPRECATION_WARNINGS',
    ],
    language: 'c',
  )
endif

meson.add_install_script('meson_install_schemas.py')

add_project_arguments(cc.get_supported_arguments(more_cflags), language: 'c')

configure_file(
  output: 'config.h',
  configuration: config,
)
rootInclude = include_directories('.')

subdir('po')
subdir('shell')
subdir('panels')

final_message = [ '',
  '',
  '** NetworkManager (Network panel)                  : @0@'.format(libnm.found()),
  '** ModemManager support                            : @0@'.format(modemmanager.found()),
  '** Colord support (Color management panel)         : @0@'.format(colord.found()),
  '** Wacom support (Wacom panel)                     : @0@'.format(libwacom.found()),
  '** Online-Accounts support (Online-Accounts panel) : @0@'.format(goa.found()),
  '',
]
message('\n'.join(final_message))
