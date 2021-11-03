project(
    'ibm-panel',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++20',
        # note: This is allowed in certain contexts, but provides binary size
        # improvements for applications that don't require rtti
        'cpp_rtti=false',
    ],
    license: 'Apache-2.0',
    version: '0.1',
    meson_version: '>= 0.57',
)

systemd = dependency('systemd')
sdbusplus = dependency('sdbusplus')

cxx = meson.get_compiler('cpp')
add_project_arguments(
cxx.get_supported_arguments([
'-DBOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT',
'-DBOOST_ASIO_DISABLE_THREADS',
'-DBOOST_NO_RTTI',
'-DBOOST_NO_TYPEID',
'-DBOOST_ALLOW_DEPRECATED_HEADERS'
]),
language : 'cpp')
add_global_arguments('-Wno-psabi', language : ['c', 'cpp'])

systemd_system_unit_dir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

service_file = 'service_files/com.ibm.panel.service'

if get_option('system-vpd-dependency').enabled()
 service_file = 'service_files/ibm.panel.system.vpd.dependent.service'
endif

configure_file(input: service_file,
               output: 'com.ibm.panel.service',
               install_dir: systemd_system_unit_dir,
               copy: true,
               install: true)

panel_app_a = static_library(
    'ibm_panel_a',
    'src/bus_handler.cpp',
    'src/panel_state_manager.cpp',
    'src/i2c_message_encoder.cpp',
    'src/button_handler.cpp',
    'src/transport.cpp',
    'src/utils.cpp',
    'src/bus_monitor.cpp',
    'src/executor.cpp',
    'src/pldm_fw.cpp',
    include_directories: 'include'
)

executable(
    'ibm-panel',
    'src/panel_app_main.cpp',
    dependencies: [
      sdbusplus,
      dependency('libpldm')
    ],
    include_directories: 'include',
    install: true,
    link_with: [
        panel_app_a,
    ],
)

if get_option('tests').enabled()
  gtest = dependency('gtest', main: true)
  gmock = dependency('gmock')
  panel_app_test = executable(
      'panel-app-test',
      'test/panel_app_test.cpp',
      'test/panel_state_manager_test.cpp',
      'test/i2c_message_encoder_test.cpp',
      dependencies: [
          sdbusplus,
          gmock,
          gtest,
      ],
      include_directories: [
          'include',
      ],
      link_with: [
          panel_app_a,
      ],
      cpp_args: ['-DStateManagerTest']
  )

  test('test_panel_app', panel_app_test)
endif
