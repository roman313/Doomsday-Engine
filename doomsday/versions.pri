# The Doomsday Engine Project
# Copyright (c) 2011-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
#
# Runs a Python script that reads the current version numbers and release
# information from the headers of the various components. Python is assumed to
# be on the system path.

defineReplace(findVersion) {
    info = $$system(python \"$$PWD/../distrib/build_version.py\" \"$$PWD/$$1\")
    # Result: versionbase buildnum reltype winver
    # Just use the base version (x.y.z) for now.
    return($$member(info, 0))
}

defineReplace(dengReleaseType) {
    info = $$system(python \"$$PWD/../distrib/build_version.py\"  \"$$PWD/api/dd_version.h\")
    return($$member(info, 2))
}

defineTest(isStableRelease) {
    relType = $$dengReleaseType()
    contains(relType, Stable): return(true)
    else: return(false)
}

DENG_VERSION            = $$findVersion(api/dd_version.h)

JDOOM_VERSION           = $$findVersion(api/dd_version.h)
JHERETIC_VERSION        = $$findVersion(api/dd_version.h)
JHEXEN_VERSION          = $$findVersion(api/dd_version.h)

DEHREAD_VERSION         = $$findVersion(plugins/dehread/include/version.h)
WADMAPCONVERTER_VERSION = $$findVersion(plugins/wadmapconverter/include/version.h)
DIRECTSOUND_VERSION     = $$findVersion(plugins/directsound/include/version.h)
OPENAL_VERSION          = $$findVersion(plugins/openal/include/version.h)
FMOD_VERSION            = $$findVersion(plugins/fmod/include/version.h)
WINMM_VERSION           = $$findVersion(plugins/winmm/include/version.h)
EXAMPLE_VERSION         = $$findVersion(plugins/example/include/version.h)

JDOOM64_VERSION         = $$findVersion(api/dd_version.h)
