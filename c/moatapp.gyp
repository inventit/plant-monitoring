{ 'includes': [
    'common.gypi',
    'config.gypi',
  ],
  'targets': [
    # your M2M/IoT application
    {
      'target_name': '<(package_name)',
      'sources': [
        'src/device-framework/io.c',
        'src/device-framework/device.c',
        'src/device-framework/serial_port.c',
        'src/device-framework/device_finder.c',
        'src/data_collector.c',
        'src/image_collector.c',
        'src/<(package_name).c',
       ],
      'product_prefix': '',
      'type': 'shared_library',
      'cflags': [ '-fPIC' ],
      'include_dirs' : [
      ],
      'libraries': [
        '-ludev',
        '-lmoatapp',
      ],
      'dependencies': [
      ],
    },

  ],
}
