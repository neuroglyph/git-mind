project('git-mind', 'c',
  version : '0.0.1',
  default_options : ['warning_level=0',  # We'll add our own flags
                     'c_std=c2x'])

# Linus-worthy C23 compiler flags - no warnings, no excuses
add_project_arguments(
  '-std=c2x',
  '-pedantic',
  '-Wall',
  '-Wextra',
  '-Werror',
  '-Wvla',                    # Ban variable-length arrays
  '-Wshadow',                 # No variable shadowing
  '-Wstrict-prototypes',      # Require proper prototypes
  '-Wconversion',             # Catch implicit conversions
  '-Wsign-conversion',        # Catch sign conversions
  '-Wfloat-equal',           # No float equality comparisons
  '-Wpointer-arith',         # Arithmetic on void pointers is stupid
  '-Wcast-align',            # Catch alignment issues
  '-Wwrite-strings',         # String literals are const
  '-Wmissing-prototypes',    # Every function needs a prototype
  '-Wmissing-declarations',  # No implicit declarations
  '-Wredundant-decls',       # No duplicate declarations
  '-Wnested-externs',        # No nested extern declarations
  '-Winline',                # Failed inline is a bug
  '-O2',                     # Optimize for real-world use
  '-fno-common',             # No common symbols
  '-fstrict-aliasing',       # We follow aliasing rules
  language : 'c'
)

# Dependencies
libsodium_dep = dependency('libsodium', required : true)
libgit2_dep = dependency('libgit2', required : true)
thread_dep = dependency('threads')

# Source files
src = files(
  'core/src/cbor/cbor.c',
  'core/src/crypto/backend.c',
  'core/src/crypto/random.c',
  'core/src/crypto/sha256.c',
  'core/src/error/error.c',
  'core/src/io/io.c',
  'core/src/time/time.c',
  'core/src/types/id.c',
  'core/src/types/path.c',
  'core/src/types/string.c',
  'core/src/types/string_core.c',
  'core/src/types/string_utf8.c',
  'core/src/types/ulid.c',
  'core/src/utf8/validate.c',
  'core/src/edge/edge.c',
  'core/src/edge/attributed.c',
  'core/src/attribution/attribution.c',
  'core/src/journal/reader.c',
  'core/src/journal/writer.c',
)

# Include directories
inc = include_directories('core/include', 'include')

# Build static library
libgitmind = static_library(
  'gitmind',
  src,
  include_directories : inc,
  dependencies : [libsodium_dep, libgit2_dep, thread_dep]
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

test_ulid = executable('test_ulid',
  'core/tests/unit/test_ulid.c',
  include_directories : inc,
  link_with : [libgitmind, test_backend_lib],
  dependencies : [libsodium_dep, thread_dep],
  c_args : ['-DGITMIND_ENABLE_TEST_BACKEND'])

test_io = executable('test_io',
  'core/tests/unit/test_io.c',
  include_directories : inc,
  link_with : libgitmind,
  dependencies : [libsodium_dep, thread_dep])

test_time = executable('test_time',
  'core/tests/unit/test_time.c',
  include_directories : inc,
  link_with : libgitmind,
  dependencies : [libsodium_dep, thread_dep])

test_cbor = executable('test_cbor',
  'core/tests/unit/test_cbor.c',
  include_directories : inc,
  link_with : libgitmind,
  dependencies : [libsodium_dep, thread_dep])

test_edge = executable('test_edge',
  'core/tests/unit/test_edge.c',
  include_directories : inc,
  link_with : libgitmind,
  dependencies : [libsodium_dep, thread_dep])

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
test('ulid', test_ulid)
test('io', test_io)
test('time', test_time)
test('cbor', test_cbor)
test('edge', test_edge)
