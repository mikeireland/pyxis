from conan import ConanFile
from conan.tools.cmake import CMake

class Pkg(ConanFile):
    settings = 'os', 'compiler', 'arch', 'build_type'
    generators = 'CMakeToolchain', 'CMakeDeps'

    requires = 'commander/0.2.0'

    def build(self):
        cmake = CMake(self)

        if self.should_configure:
            cmake.configure()
        if self.should_build:
            cmake.build()
