project('git-mind', 'c',
  version : '0.0.1',
  default_options : ['warning_level=0',  # We'll add our own flags
                     'c_std=gnu11'])

# Add specific compiler flags to match original Makefile exactly
add_project_arguments('-Wall', '-Wextra', '-Werror', '-O2', language : 'c')

# Dependencies
libsodium_dep = dependency('libsodium', required : true)
thread_dep = dependency('threads')

# Source files
src = files(
  'core/src/crypto/backend.c',
  'core/src/crypto/random.c',
  'core/src/crypto/sha256.c',
  'core/src/error/error.c',
  'core/src/types/id.c',
  'core/src/types/path.c',
  'core/src/types/string.c',
  'core/src/types/string_core.c',
  'core/src/types/string_utf8.c',
  'core/src/utf8/validate.c',
)

# Include directories
inc = include_directories('core/include')

# Build static library
libgitmind = static_library(
  'gitmind',
  src,
  include_directories : inc,
  dependencies : [libsodium_dep, thread_dep]
)

# Test backend - ONLY for tests
test_backend_src = files('core/tests/backends/test_backend.c')
test_backend_lib = static_library(
  'gitmind_test_backend',
  test_backend_src,
  include_directories : inc,
  dependencies : [libsodium_dep],
  c_args : ['-DGITMIND_ENABLE_TEST_BACKEND']
)

# Individual test executables (matching original Makefile)
test_error = executable('test_error',
  'core/tests/unit/test_error.c',
  include_directories : inc,
  link_with : libgitmind,
  dependencies : [libsodium_dep, thread_dep])

test_id = executable('test_id',
  'core/tests/unit/test_id.c',
  include_directories : inc,
  link_with : libgitmind,
  dependencies : [libsodium_dep, thread_dep])

test_id_thread_safety = executable('test_id_thread_safety',
  'core/tests/unit/test_id_thread_safety.c',
  include_directories : inc,
  link_with : libgitmind,
  dependencies : [libsodium_dep, thread_dep])

test_string = executable('test_string',
  'core/tests/unit/test_string.c',
  include_directories : inc,
  link_with : libgitmind,
  dependencies : [libsodium_dep, thread_dep])

test_path = executable('test_path',
  'core/tests/unit/test_path.c',
  include_directories : inc,
  link_with : libgitmind,
  dependencies : [libsodium_dep, thread_dep])

test_sha256 = executable('test_sha256',
  'core/tests/unit/test_sha256.c',
  include_directories : inc,
  link_with : [libgitmind, test_backend_lib],
  dependencies : [libsodium_dep, thread_dep],
  c_args : ['-DGITMIND_ENABLE_TEST_BACKEND'])

test_random = executable('test_random',
  'core/tests/unit/test_random.c',
  include_directories : inc,
  link_with : [libgitmind, test_backend_lib],
  dependencies : [libsodium_dep, thread_dep],
  c_args : ['-DGITMIND_ENABLE_TEST_BACKEND'])

test_utf8 = executable('test_utf8',
  'core/tests/unit/test_utf8.c',
  include_directories : inc,
  link_with : libgitmind,
  dependencies : [libsodium_dep, thread_dep])

test_crypto_backend = executable('test_crypto_backend',
  'core/tests/unit/test_crypto_backend.c',
  include_directories : inc,
  link_with : [libgitmind, test_backend_lib],
  dependencies : [libsodium_dep, thread_dep],
  c_args : ['-DGITMIND_ENABLE_TEST_BACKEND'])

# Register tests
test('error', test_error)
test('id', test_id)
test('id_thread_safety', test_id_thread_safety)
test('string', test_string)
test('path', test_path)
test('sha256', test_sha256)
test('random', test_random)
test('utf8', test_utf8)
test('crypto_backend', test_crypto_backend)