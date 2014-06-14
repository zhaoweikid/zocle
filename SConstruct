import os, sys

name		= 'zocle'
includes	= ['include']
libs		= ['z']
libpath		= ['.', '/usr/local/lib']
ldflags		= '-pg'
msys_home	= os.environ.get('MINGW_HOME', '')

defs = ['_REENTRANT', 
		'_GNU_SOURCE', 
		'ZOCLE_WITH_ICONV', 
		'ZOCLE_WITH_PCRE', 
		'ZOCLE_WITH_LIBEV', 
		'ZOCLE_WITH_SSL', 
		'ZOCLE_WITH_SQLITE', 
		#'ZOCLE_WITH_MYSQL',
		#'ASYNC_ONE_WATCHER',
		#'ZOCLE_WITH_TCMALLOC',
		]

files = []
for root,dirs,fs in os.walk('src'):
	for d in dirs:
		p = '%s/%s/*.c' % (root, d)
		files += Glob(p)


if sys.platform.startswith('win'):
	includes.append(os.path.join(msys_home, 'local/include'))
	libpath.append(os.path.join(msys_home, 'local/lib'))

if 'ZOCLE_WITH_SSL' in defs:
	if os.path.isdir('/usr/include/openssl'):
		print 'use ssl at /usr/include/openssl'
	elif os.path.isdir('/usr/local/ssl'):
		print 'use ssl at /usr/local/ssl'
		if sys.platform.startswith('win'):
			includes.append(os.path.join(msys_home, 'local/ssl/include'))
			libpath.append(os.path.join(msys_home, 'local/ssl/lib'))
		else:
			includes.append('/usr/local/ssl/include')
			libpath.append('/usr/local/ssl/lib')
	libs.append('ssl')
libs.append('crypto')

if 'ZOCLE_WITH_ICONV' in defs:
	libs.append('iconv')

if 'ZOCLE_WITH_PCRE' in defs:
	libs.append('pcre')

if 'ZOCLE_WITH_LIBEV' in defs:
	libs.append('ev')

if 'ZOCLE_WITH_MYSQL' in defs:
	libs.append('mysqlclient')
	if os.path.isdir('/usr/local/mysql'):
		includes.append('/usr/local/mysql/include')
		libpath.append('/usr/local/mysql/lib')
	elif os.path.isdir('/opt/mysql'):
		includes.append('/opt/mysql/include')
		libpath.append('/opt/mysql/lib')

if 'ZOCLE_WITH_SQLITE' in defs:
	includes.append('/usr/local/include')
	libs.append('sqlite3')

if 'ZOCLE_WITH_TCMALLOC' in defs:
	libs.append('tcmalloc_minimal')

if 'ZOCLE_WITH_GC' in defs:
	libs.append('gc')


env = None
if sys.platform == 'win32':
	libs += ['ws2_32', 'pthreadGC2', 'msvcrt', 'gdi32']
	env = Environment(tools=['mingw'], CCFLAGS='-ggdb -std=gnu99 -Wall -pg', 
				CPPDEFINES=defs, CPPPATH=includes, 
				LIBPATH=libpath, LIBS=libs, LINKFLAGS=ldflags)
else:
	env = Environment(CCFLAGS='-ggdb -pg -std=gnu99 -Wall', 
				CPPDEFINES=defs, CPPPATH=includes, 
				LIBPATH=libpath, LIBS=libs, LINKFLAGS=ldflags)

env.StaticLibrary(name, files)
env.SharedLibrary(name, files)

#SConscript('test/SConstruct', exports=['defs', 'includes', 'libpath', 'libs'])

