deps = [
  libwacom,
  cinn_desktop,
  gtk,
  libx11,
  libxi
]

muffin_display_iface_sources = gnome.gdbus_codegen(
  'muffin-display-config',
  'muffin-display-config.xml',
  namespace: 'MetaDBus',
  interface_prefix: 'org.cinnamon.Muffin'
)

test_cflags = more_cflags + ['-DFAKE_AREA']

wacom_gresource = gnome.compile_resources(
  'cc-wacom-resources',
  'cinnamon-wacom.gresource.xml',
    source_dir : '.',
        c_name : 'cc_wacom',
  dependencies : resource_data,
        export : true
)

subdir('calibrator')

install_data(
  'cinnamon-wacom-panel.desktop',
  install_dir: desktop_dir
)

common_sources = [
  muffin_display_iface_sources,
  'cc-tablet-tool-map.c',
  'cc-wacom-button-row.c',
  'cc-wacom-device.c',
  'cc-wacom-mapping-panel.c',
  'cc-wacom-nav-button.c',
  'cc-wacom-output-manager.c',
  'cc-wacom-page.c',
  'cc-wacom-stylus-page.c',
  'cc-wacom-tool.c',
  'csd-wacom-key-shortcut-button.c'
]

resource_data = files(
  'calibrator/calibrator.ui',
  'calibrator/calibrator.css',
  'calibrator/target.svg',
  'button-mapping.ui',
  'cinnamon-wacom-properties.ui',
  'wacom-stylus-3btn.svg',
  'wacom-stylus-3btn-no-eraser.svg',
  'wacom-stylus-airbrush.svg',
  'wacom-stylus-art-pen.svg',
  'wacom-stylus-classic.svg',
  'wacom-stylus-inking.svg',
  'wacom-stylus-no-eraser.svg',
  'wacom-stylus-page.ui',
  'wacom-stylus.svg',
  'wacom-tablet-cintiq.svg',
  'wacom-tablet-pc.svg',
  'wacom-tablet.svg'
)

common_sources += wacom_gresource

sources = common_sources + files(
  'cc-wacom-panel.c',
  'cc-drawing-area.c',
  'wacom-module.c'
)

deps += libdevice_dep

incs = [
  rootInclude,
  calibrator_inc
]

panel_wacom = shared_library('wacom-properties',
              sources : sources,
  include_directories : incs,
         dependencies : deps,
           link_whole : [ libwacom_calibrator ],
             link_with: [libcinnamon_control_center],
               install: true,
           install_dir: panels_dir
)

sources += 'test-wacom.c'

executable('test-wacom',
  sources,
  include_directories : incs,
  dependencies : deps,
  c_args : test_cflags,
  link_with : [ libwacom_calibrator_test, libcinnamon_control_center ]
)
