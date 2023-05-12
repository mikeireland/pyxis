from conans import ConanFile
from conan.tools.cmake import CMake

class CommanderConan(ConanFile):
    name = 'commander'
    version = '0.2.0'
    license = "MIT"

    build_policy = 'missing'
    no_copy_source = True
    exports_sources = 'CMakeLists.txt', 'include/*', 'src/*'

    requires = [
        'nlohmann_json/3.10.5',
        'cppzmq/4.8.1',
        'boost/1.78.0',
        'fmt/8.1.1',
    ]

    options = {
        'shared' : [True, False],
        'fPIC'   : [True, False]
    }

    default_options = {
        'shared' : False,
        'fPIC'   : True
    }

    settings = 'os', 'compiler', 'build_type', 'arch'
    generators = 'CMakeToolchain', 'CMakeDeps'

    def build(self):
        cmake = CMake(self)

        if self.should_configure:
            cmake.configure()
        if self.should_build:
            cmake.build()

    def package(self):
        cmake = CMake(self)

        if self.should_install:
            cmake.install()

    def package_info(self):
        self.cpp_info.libs   = ['commander']
