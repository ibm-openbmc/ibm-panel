project(
    'ibm-panel',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++23',
        # note: This is allowed in certain contexts, but provides binary size
        # improvements for applications that don't require rtti
        'cpp_rtti=false',
    ],
    license: 'Apache-2.0',
    version: '0.1',
    meson_version: '>= 1.1.1',
)

systemd = dependency('systemd')
sdbusplus = dependency('sdbusplus')
phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
phosphor_logging = dependency('phosphor-logging')

cxx = meson.get_compiler('cpp')
add_project_arguments(
    cxx.get_supported_arguments(
        [
            '-DBOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT',
            '-DBOOST_ASIO_DISABLE_THREADS',
            '-DBOOST_NO_RTTI',
            '-DBOOST_NO_TYPEID',
            '-DBOOST_ALLOW_DEPRECATED_HEADERS',
        ],
    ),
    language: 'cpp',
)
add_global_arguments('-Wno-psabi', language: ['c', 'cpp'])

systemd_system_unit_dir = systemd.get_variable('systemdsystemunitdir')

service_file = 'service_files/com.ibm.panel.service'

if get_option('system-vpd-dependency').enabled()
    service_file = 'service_files/ibm.panel.system.vpd.dependent.service'
endif

configure_file(
    input: service_file,
    output: 'com.ibm.panel.service',
    install_dir: systemd_system_unit_dir,
    copy: true,
    install: true,
)

common_SOURCES = ['src/transport.cpp', 'src/panel_state_manager.cpp']

panel_SOURCE = ['src/main.cpp'] + common_SOURCES

executable(
    'ibm-panel',
    panel_SOURCE,
    dependencies: [sdbusplus, phosphor_dbus_interfaces, phosphor_logging],
    include_directories: ['include'],
    install: true,
)
