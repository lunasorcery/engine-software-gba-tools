project('engine-software-gba-tools', 'cpp',
	version: '0.0.1',
	default_options : ['cpp_std=c++17'])

fmt_dep = subproject('fmt').get_variable('fmt_dep')

src_shared = ['src/common-xm.cpp', 'src/common-gba.cpp', 'src/misc.cpp']

executable('gba2xm',   sources: ['src/gba2xm.cpp',   src_shared], dependencies: fmt_dep)
executable('gbafind',  sources: ['src/gbafind.cpp',  src_shared], dependencies: fmt_dep)
executable('gbaprint', sources: ['src/gbaprint.cpp', src_shared], dependencies: fmt_dep)
executable('xmprint',  sources: ['src/xmprint.cpp',  src_shared], dependencies: fmt_dep)
