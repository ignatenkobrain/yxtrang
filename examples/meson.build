l_args = ['-pthread', '-lm']
deps = [dependency('openssl')]

test = executable(
  'testt',
  sources : 'test.c',
  link_with : lib,
  include_directories : inc,
  link_args : l_args,
  dependencies : deps
)
test('testt', test)

echo1 = executable(
  'echo1',
  sources : 'echo1.c',
  link_with : lib,
  include_directories : inc,
  link_args : l_args,
  dependencies : deps
)

echo2 = executable(
  'echo2',
  sources : 'echo2.c',
  link_with : lib,
  include_directories : inc,
  link_args : l_args,
  dependencies : deps
)

echod = executable(
  'echod',
  sources : 'echod.c',
  link_with : lib,
  include_directories : inc,
  link_args : l_args,
  dependencies : deps
)

httpd = executable(
  'httpd',
  sources : 'httpd.c',
  link_with : lib,
  include_directories : inc,
  link_args : l_args,
  dependencies : deps
)

lindad = executable(
  'lindad',
  sources : 'lindad.c',
  link_with : lib,
  include_directories : inc,
  link_args : l_args,
  dependencies : deps
)
#test('lindad', lindad) # FIXME: fails to run
