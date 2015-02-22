
backend = SConscript('Backend/SConstruct')

env = Environment()
xbuild = env.Command('dummy',[backend], 'xcodebuild -target "voxel1Editor - Mac"')
env.AlwaysBuild(xbuild)