project('git-mind', 'c',
        version: '0.6.0',
        default_options: ['c_std=c23', 'warning_level=3', 'werror=true'])

# Optional feature toggles
option('enable_io', type: 'boolean', value: false, description: 'Enable IO public headers')
option('enable_time', type: 'boolean', value: false, description: 'Enable time public headers')
option('enable_util', type: 'boolean', value: false, description: 'Enable util public headers')
option('enable_utf8', type: 'boolean', value: true, description: 'Enable UTF-8 public headers')

# HARD E-BRAKE: refuse host builds unless in Docker or CI
in_docker = run_command('sh', '-lc', '[ "$GITMIND_DOCKER" = "1" ] && echo 1 || echo 0').stdout().strip() == '1'
in_ci = run_command('sh', '-lc', '[ -n "$GITHUB_ACTIONS" ] && echo 1 || echo 0').stdout().strip() == '1'
override = run_command('sh', '-lc', '[ "$GITMIND_ALLOW_HOST_BUILD" = "1" ] && echo 1 || echo 0').stdout().strip() == '1'
if not in_docker and not in_ci and not override
  error('\n\n💥 Build refused: git-mind must be built in Docker.\n\n' +
        'Why: this project touches Git internals; host builds can corrupt your repo.\n' +
        'Do this instead:\n' +
        '  make dev           # open dev shell in CI container\n' +
        '  make build-docker  # build in container\n' +
        '  make test-docker   # test in container\n\n' +
        'If you REALLY know what you are doing, set GITMIND_ALLOW_HOST_BUILD=1.\n')
endif

# C23 compiler flags as specified
add_project_arguments(
  '-ffreestanding',          # library and want tight control
  '-fstrict-flex-arrays=3',  # C23 default; makes UB obvious
  '-Wdouble-promotion',
  '-Wpedantic',
  # Note: -Werror is set via werror=true in project() default_options
  '-Wvla',                   # Ban variable-length arrays
  '-Wshadow',                # No variable shadowing
  '-Wstrict-prototypes',     # Require proper prototypes
  '-Wconversion',            # Catch implicit conversions
  '-Wsign-conversion',       # Catch sign conversions
  '-Wfloat-equal',          # No float equality comparisons
  '-Wpointer-arith',        # Arithmetic on void pointers is stupid
  '-Wcast-align',           # Catch alignment issues
  '-Wwrite-strings',        # String literals are const
  '-Wmissing-prototypes',   # Every function needs a prototype
  '-Wmissing-declarations', # No implicit declarations
  '-Wredundant-decls',      # No duplicate declarations
  '-Wnested-externs',       # No nested extern declarations
  '-Winline',               # Failed inline is a bug
  '-fno-common',            # No common symbols
  '-fstrict-aliasing',      # We follow aliasing rules
  language : 'c'
)

# Define feature toggles globally
f_cargs = []
if get_option('enable_io')
  f_cargs += ['-DGITMIND_ENABLE_IO']
endif
if get_option('enable_time')
  f_cargs += ['-DGITMIND_ENABLE_TIME']
endif
if get_option('enable_util')
  f_cargs += ['-DGITMIND_ENABLE_UTIL']
endif
if get_option('enable_utf8')
  f_cargs += ['-DGITMIND_ENABLE_UTF8']
endif
add_project_arguments(f_cargs, language: 'c')

# Dependencies
libsodium_dep = dependency('libsodium', required : true)
libgit2_dep = dependency('libgit2', required : true)
thread_dep = dependency('threads')
# CRoaring (Roaring Bitmaps) dependency - robust detection
roaring_dep = dependency('roaring', required : false)
if not roaring_dep.found()
  roaring_dep = dependency('croaring', required : false)
endif
if not roaring_dep.found()
  roaring_dep = dependency('CRoaring', required : false)
endif
if not roaring_dep.found()
  error('CRoaring/roaring library is required but not found.\n' +
        'Install via package manager:\n' +
        '  Ubuntu/Debian: apt install libroaring-dev\n' +
        '  Fedora/RHEL: dnf install CRoaring-devel\n' +
        '  macOS: brew install croaring\n' +
        '  Or build from source: https://github.com/RoaringBitmap/CRoaring\n' +
        '\n' +
        'Note: Different distributions may name the pkg-config file differently.')
endif

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
  'core/src/cache/bitmap.c',
  'core/src/cache/builder.c',
  'core/src/cache/query.c',
  'core/src/cache/tree_builder.c',
  'core/src/cache/tree_size.c',
)

# Include directories
inc = include_directories('core/include', 'include')

# Mark roaring headers as system includes to suppress third-party warnings
roaring_inc_args = []
if roaring_dep.found()
  # Use modern API that works with any dependency type
  roaring_inc_dir = roaring_dep.get_variable(pkgconfig: 'includedir', default_value: '')
  if roaring_inc_dir != ''
    roaring_inc_args = ['-isystem', roaring_inc_dir]
  endif
endif

# Build static library
libgitmind = static_library(
  'gitmind',
  src,
  include_directories : inc,
  dependencies : [libsodium_dep, libgit2_dep, thread_dep, roaring_dep],
  c_args : roaring_inc_args
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

test_edge_cbor_oid = executable('test_edge_cbor_oid',
  'core/tests/unit/test_edge_cbor_oid.c',
  include_directories : inc,
  link_with : libgitmind,
  dependencies : [libsodium_dep, thread_dep])

test_edge_attributed_cbor = executable('test_edge_attributed_cbor',
  'core/tests/unit/test_edge_attributed_cbor.c',
  include_directories : inc,
  link_with : libgitmind,
  dependencies : [libsodium_dep, thread_dep])

test_journal_safety = executable('test_journal_safety',
  'core/tests/unit/test_journal_safety.c',
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
test('edge_cbor_oid', test_edge_cbor_oid)
test('edge_attributed_cbor', test_edge_attributed_cbor)
test('journal_safety', test_journal_safety)

# Standalone header compilation checks (public headers)
header_check_src = 'tools/header_compile.c'
public_headers = [
  'include/gitmind/output.h',
  'include/gitmind/constants.h',
  'include/gitmind/error.h',
  'include/gitmind/safety.h',
  'core/include/gitmind/result.h',
  'core/include/gitmind/types.h',
  'core/include/gitmind/context.h',
  'core/include/gitmind/attribution.h',
  'core/include/gitmind/edge.h',
  'core/include/gitmind/edge_attributed.h',
  'core/include/gitmind/journal.h',
  'core/include/gitmind/cache.h',
  'core/include/gitmind/io/io.h',
  'core/include/gitmind/time/time.h',
  'core/include/gitmind/util/memory.h',
  'core/include/gitmind/utf8/validate.h',
]

foreach hdr : public_headers
  exe_name = 'hdr_' + hdr.split('/')[-1].replace('.', '_')
  executable(exe_name,
    header_check_src,
    include_directories : inc,
    dependencies : [libsodium_dep, libgit2_dep, thread_dep, roaring_dep],
    c_args : ['-include', hdr],
  )
endforeach

# Lint: header guard prefix check
run_target('lint_header_guards',
  command : ['python3', 'tools/quality/check_header_guards.py'])
