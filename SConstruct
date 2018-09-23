
import sys
import os
import multiprocessing
import SCons.Util

## Configuration

opts = Variables('Local.sc')

opts.AddVariables(
    ("CC", "C Compiler", "cc"),
    ("CXX", "C++ Compiler", "c++"),
    ("AS", "Assembler"),
    ("LINK", "Linker"),
    ("CLANGTIDY", "Clang checker", "clang-tidy39"),
    ("CTAGS", "Ctags", "exctags"),
    ("NUMCPUS", "Number of CPUs to use for build (0 means auto)", 0, None, int),
    EnumVariable("BUILDTYPE", "Build type", "RELEASE", ["RELEASE", "DEBUG", "PERF"]),
    BoolVariable("VERBOSE", "Show full build information", 0),
    BoolVariable("WITH_GPROF", "Include gprof profiling", 0),
    BoolVariable("WITH_GOOGLEHEAP", "Link to Google Heap Cheker", 0),
    BoolVariable("WITH_GOOGLEPROF", "Link to Google CPU Profiler", 0),
    BoolVariable("WITH_TSAN", "Enable Clang Race Detector", 0),
    BoolVariable("WITH_ASAN", "Enable Clang AddressSanitizer", 0),
    PathVariable("PREFIX", "Installation target directory", "/usr/local/bin/", PathVariable.PathAccept),
    PathVariable("DESTDIR", "The root directory to install into. Useful mainly for binary package building", "", PathVariable.PathAccept),
)

env = Environment(options = opts,
                  tools = ['default'],
                  ENV = os.environ)
Help("""TARGETS:
scons               Build castor
scons testbench     Run tests
scons tags          Ctags\n""")
Help(opts.GenerateHelpText(env))

# Copy environment variables
if os.environ.has_key('CC'):
    env["CC"] = os.getenv('CC')
if os.environ.has_key('CXX'):
    env["CXX"] = os.getenv('CXX')
if os.environ.has_key('AS'):
    env["AS"] = os.getenv('AS')
if os.environ.has_key('LD'):
    env["LINK"] = os.getenv('LD')
if os.environ.has_key('CFLAGS'):
    env.Append(CCFLAGS = SCons.Util.CLVar(os.environ['CFLAGS']))
if os.environ.has_key('CPPFLAGS'):
    env.Append(CPPFLAGS = SCons.Util.CLVar(os.environ['CPPFLAGS']))
if os.environ.has_key('CXXFLAGS'):
    env.Append(CXXFLAGS = SCons.Util.CLVar(os.environ['CXXFLAGS']))
if os.environ.has_key('LDFLAGS'):
    env.Append(LINKFLAGS = SCons.Util.CLVar(os.environ['LDFLAGS']))
env["ENV"].update(x for x in os.environ.items() if x[0].startswith("CCC_"))

env.Append(CPPFLAGS = [ "-Wall", "-Wformat=2", "-Wextra", "-Wwrite-strings",
                        "-Wno-unused-parameter", "-Wmissing-format-attribute",
                        "-Werror" ])
env.Append(CFLAGS = [ "-Wmissing-prototypes", "-Wmissing-declarations",
                      "-Wshadow", "-Wbad-function-cast", "-Werror" ])
env.Append(CXXFLAGS = [ "-Wno-unsupported-friend", "-Woverloaded-virtual",
                        "-Wcast-qual", "-Wcast-align", "-Wconversion",
                        "-Weffc++", "-std=c++0x", "-Werror" ])

if env["WITH_GPROF"]:
    env.Append(CPPFLAGS = [ "-pg" ])
    env.Append(LINKFLAGS = [ "-pg" ])

env.Append(CFLAGS = ["-std=c11"])
env.Append(CXXFLAGS = ["-std=c++17"])
env.Append(CPPFLAGS = ["-Wall", "-Werror" ])
env.Append(CPPFLAGS = ["-Wno-potentially-evaluated-expression" ])
 # "-Wsign-compare", "-Wsign-conversion", "-Wcast-align"

if env["BUILDTYPE"] == "DEBUG":
    env.Append(CPPFLAGS = [ "-g", "-DCELESTIS_DEBUG", "-O0" ])
    env.Append(LINKFLAGS = [ "-g", "-rdynamic" ])
elif env["BUILDTYPE"] == "PERF":
    env.Append(CPPFLAGS = [ "-g", "-DNDEBUG", "-DCELESTIS_PERF", "-O2"])
    env.Append(LDFLAGS = [ "-g", "-rdynamic" ])
elif env["BUILDTYPE"] == "RELEASE":
    env.Append(CPPFLAGS = ["-DNDEBUG", "-DCELESTIS_RELEASE", "-O2"])
else:
    print "Error BUILDTYPE must be RELEASE or DEBUG"
    sys.exit(-1)

if not env["VERBOSE"]:
    env["CCCOMSTR"] = "Compiling $SOURCE"
    env["CXXCOMSTR"] = "Compiling $SOURCE"
    env["SHCCCOMSTR"] = "Compiling $SOURCE"
    env["SHCXXCOMSTR"] = "Compiling $SOURCE"
    env["ARCOMSTR"] = "Creating library $TARGET"
    env["RANLIBCOMSTR"] = "Indexing library $TARGET"
    env["LINKCOMSTR"] = "Linking $TARGET"
    env["SHLINKCOMSTR"] = "Linking $TARGET"
    env["ASCOMSTR"] = "Assembling $TARGET"
    env["ASPPCOMSTR"] = "Assembling $TARGET"
    env["ARCOMSTR"] = "Creating library $TARGET"
    env["RANLIBCOMSTR"] = "Indexing library $TARGET"

def GetNumCPUs(env):
    if env["NUMCPUS"] > 0:
        return int(env["NUMCPUS"])
    return 2*multiprocessing.cpu_count()

env.SetOption('num_jobs', GetNumCPUs(env))

# Modify CPPPATH and LIBPATH
env.Append(CPPPATH = [ "/usr/local/include" ])
env.Append(LIBPATH = [ "$LIBPATH", "/usr/local/lib" ])

# FreeBSD requires libexecinfo
# Linux and darwin have the header
if sys.platform.startswith("freebsd"):
    env.Append(LIBS = ['execinfo'])
    env.Append(CPPFLAGS = "-DHAVE_EXECINFO")
elif sys.platform == "linux2" or sys.platform == "darwin":
    env.Append(CPPFLAGS = "-DHAVE_EXECINFO")

# XXX: Hack to support clang static analyzer
def CheckFailed():
    if os.getenv('CCC_ANALYZER_OUTPUT_FORMAT') != None:
        return
    Exit(1)

# Configuration
conf = env.Configure()

if not conf.CheckCC():
    print 'Your C compiler and/or environment is incorrectly configured.'
    CheckFailed()

env.AppendUnique(CXXFLAGS = ['-std=c++17'])
if not conf.CheckCXX():
    print 'Your C++ compiler and/or environment is incorrectly configured.'
    CheckFailed()

conf.Finish()

Export('env')

if sys.platform != "win32" and sys.platform != "darwin":
    env.Append(CPPFLAGS = ['-pthread'])
    env.Append(LIBS = ["pthread"])

# Debugging Tools
if env["WITH_GOOGLEHEAP"]:
    env.Append(LIBS = ["tcmalloc"])
if env["WITH_GOOGLEPROF"]:
    env.Append(LIBS = ["profiler"])
if env["WITH_TSAN"]:
    env.Append(CPPFLAGS = ["-fsanitize=thread", "-fPIE"])
    env.Append(LINKFLAGS = ["-fsanitize=thread", "-pie"])
if env["WITH_ASAN"]:
    env.Append(CPPFLAGS = ["-fsanitize=address"])
    env.Append(LINKFLAGS = ["-fsanitize=address"])
if env["WITH_TSAN"] and env["WITH_ASAN"]:
    print "Cannot set both WITH_TSAN and WITH_ASAN!"
    sys.exit(-1)

# Libraries
SConscript('lpr-music/SConscript', variant_dir='build/lpr-music')
SConscript('speakerd/SConscript', variant_dir='build/speakerd')

if ("tags" in BUILD_TARGETS):
    env.Command("tags", ["lib", "include", "tools"],
                              '$CTAGS -R -f $TARGET $SOURCES')

