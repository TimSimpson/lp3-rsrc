import os
import conans

class TestPackage(conans.ConanFile):

    generators = "cmake_paths", "cmake_find_package"
    settings = "os", "compiler", "build_type", "arch"

    requires = (
# ~end-doc
         "Lp3-Main/1.0.2@TimSimpson/testing",
         "zlib/1.2.11",
# ~start-doc
         "Lp3-Sdl/1.0.3@TimSimpson/testing"
    )

    sdl2_requires = (
        "sdl2_image/2.0.5@bincrafters/stable",
    )

    def requirements(self):
        if self.settings.os != "Emscripten":
            for r in self.sdl2_requires:
                self.requires.add(r)

    def build(self):
        cmake = conans.CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        pass
