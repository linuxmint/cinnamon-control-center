project('cinnamon-control-center', 'c',
  version: '3.8.0',
  meson_version: '>=0.41.0',
)

i18n      = import('i18n')
pkgconfig = import('pkgconfig')
gnome     = import('gnome')
conf  = configuration_data()
cc    = meson.get_compiler('c')

###############################################################################
# Project conf

project_url = 'https://github.com/linuxmint/cinnamon-control-center'

lib_ver = '1.0.0'

gettext_package = meson.project_name()

conf.set_quoted('PACKAGE',          meson.project_name())
conf.set_quoted('VERSION',          meson.project_version())
conf.set_quoted('GETTEXT_PACKAGE',  gettext_package)

###############################################################################
# Dependencies

math = cc.find_library('m')

cinn_desktop= dependency('cinnamon-desktop')
cinn_menu   = dependency('libcinnamon-menu-3.0')

gtk         = dependency('gtk+-3.0',        version: '>=3.8.0')
glib        = dependency('glib-2.0',        version: '>=2.31.0')
libgnomekbd = dependency('libgnomekbd',     version: '>=3.0.0')
libgnomekbdui=dependency('libgnomekbdui',   version: '>=3.0.0')
libnotify   = dependency('libnotify',       version: '>=0.7.3')
libx11      = dependency('x11')
polkit_gobj = dependency('polkit-gobject-1',version: '>=0.103')

# dbus_glib   = dependency('dbus-glib-1')
# fontconfig  = dependency('fontconfig')
# gdk_pixbuf  = dependency('gdk-pixbuf-2.0',  version: '>=2.23.0')
# gio         = dependency('gio-2.0')
# gio_unix    = dependency('gio-unix-2.0')
# gmodule     = dependency('gmodule-2.0')
# gthread     = dependency('gthread-2.0')
# libxklavier = dependency('libxklavier',     version: '>=5.1')
# libxml      = dependency('libxml-2.0')

conf.set('HAVE_X11_EXTENSIONS_XKB_H', cc.has_header('X11/extensions/XKB.h'))

###############################################################################
# This is a hard-dependency for the region and user-accounts panels
isocodes = dependency('iso-codes')
conf.set_quoted('ISO_CODES_PREFIX', isocodes.get_pkgconfig_variable('prefix'))

###############################################################################
# Network Manager stuff
if get_option('networkmanager')
  libnm         = dependency('libnm', version: '>=1.2.0')
  libnma        = dependency('libnma',version: '>=1.2.0')
  networkmanager= dependency('NetworkManager')

  libnma_18 = libnm.version().version_compare('>=1.8.0')

  nm_vpn_config_dir = networkmanager.get_pkgconfig_variable('configdir') + '/VPN'
  nm_vpn_module_dir = networkmanager.get_pkgconfig_variable('plugindir')
else
  libnm = dependency('', required: false)
  libnma= dependency('', required: false)
  libnma_18 = false
endif

if get_option('modemmanager')
  if not get_option('networkmanager')
    error('*** NetworkManager is required by ModemManager ***')
  endif

  modemmanager = dependency('mm-glib', version: '>=0.7')
else
  modemmanager = dependency('', required: false)
endif

conf.set('HAVE_NMA_18',           libnma_18)
conf.set('BUILD_MODEM',           modemmanager.found())


###############################################################################
# Printing/CUPS
if get_option('cups')
  cups = dependency('cups', version: '>=1.4')
else
  cups = dependency('', required: false)
endif

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
panels_dir = join_paths(get_option('libdir'), 'cinnamon-control-center-1', 'panels')
ui_dir     = join_paths(get_option('datadir'), meson.project_name(), 'ui')
um_pix_dir = join_paths(get_option('datadir'), meson.project_name(), 'pixmaps')
menu_dir   = join_paths(get_option('sysconfdir'), 'xdg', 'menus')


prefix = get_option('prefix')
conf.set_quoted('BINDIR',             join_paths(prefix, get_option('bindir')))
conf.set_quoted('SBINDIR',            join_paths(prefix, get_option('sbindir')))
conf.set_quoted('LIBLOCALEDIR',       join_paths(prefix, get_option('libdir'), 'locale'))
conf.set_quoted('LOCALE_DIR',         join_paths(prefix, get_option('localedir')))
conf.set_quoted('CINNAMONLOCALEDIR',  join_paths(prefix, get_option('localedir')))
conf.set_quoted('DATADIR',            join_paths(prefix, get_option('datadir')))
conf.set_quoted('CINNAMONCC_DATA_DIR',join_paths(prefix, get_option('datadir')))
conf.set_quoted('CINNAMONCC_UI_DIR',  join_paths(prefix, ui_dir))
conf.set_quoted('PANELS_DIR',         join_paths(prefix, panels_dir))
conf.set_quoted('MENUDIR',            join_paths('/', menu_dir))
conf.set_quoted('UIDIR',              join_paths(prefix, ui_dir))
conf.set_quoted('UM_PIXMAP_DIR',      join_paths(prefix, um_pix_dir))

###############################################################################
# Build options


more_cflags = []

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
  # TODO
endif

if get_option('buildtype').contains('rel')
  more_cflags += [
    '-Wp,-D_FORTIFY_SOURCE=2',
  ]
endif

more_cflags += [
  '-Wno-strict-aliasing',
  '-Wno-sign-compare',
]

configure_file(
  output: 'config.h',
  configuration: conf,
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
