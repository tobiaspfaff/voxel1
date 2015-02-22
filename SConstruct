parent_call = 1

backend = SConscript('Backend/SConstruct', exports='parent_call')

env = Environment()
xbuild = env.Command('dummy',[backend], 'xcodebuild -target "voxel1Editor - Mac"')
env.AlwaysBuild(xbuild)