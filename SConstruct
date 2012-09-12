
# Get target mode release/debug
mymode = ARGUMENTS.get('mode', 'release')

# Check if the user has been naughty: only 'debug' or 'release' allowed
if not (mymode in ['debug', 'release']):
	print "Error: expected 'debug' or 'release', found: " + mymode
	Exit(1)

# Tell the user what we're doing
print '**** Compiling in ' + mymode + ' mode...'

# Extra compile flags for debug
debugcflags = ['-std=c++0x', '-g', '-D_DEBUG']
# Extra compile flags for release
releasecflags = ['-std=c++0x', '-O2', '-DNDEBUG']

env = Environment()

# Make sure the sconscripts can get to the variables
Export('env', 'mymode', 'debugcflags', 'releasecflags')

# Put all .sconsign files in one place
env.SConsignFile()

# Specify the sconscript for myprogram
project = 'deep'
SConscript(project + '/SConscript', exports=['project'])

# Specify the sconscript for hisprogram
project = 'deep_test'
SConscript(project + '/SConscript', exports=['project'])

project = 'rat2sdf'
SConscript(project + '/SConscript', exports=['project'])
