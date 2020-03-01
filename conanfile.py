import os.path

import conans


class Lp3Rsrc(conans.ConanFile):
    name = "Lp3-Rsrc"
    version = "1.0.1"
    license = "Zlib"
    author = "Tim Simpson"
    url = "https://github.com/TimSimpson/Lp3-Rsrc"
    description = "wraps file I/O"

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}

    requires = [
        "zlib/1.2.11@conan/stable",
        "fmt/6.1.2",
        "Lp3-Sdl/1.0.1@TimSimpson/testing"
    ]

    test_requires = [
        "catch2/2.4.1@bincrafters/stable",
    ]

    build_requires = (
        "catch2/2.4.1@bincrafters/stable"
    )
    generators = "cmake_paths", "cmake_find_package"

    exports_sources = (
        "src/*", "include/*", "demos/*", "tests/*", "CMakeLists.txt"
    )

    @property
    def tests_enabled(self):
        return (
            self.develop
            and (os.environ.get("CONAN_SKIP_TESTS") or "").lower() != 'true'
        )

    def _configed_cmake(self):
        cmake = conans.CMake(self)
        cmake.configure(defs={
            "CMAKE_FIND_PACKAGE_PREFER_CONFIG": True,
            "LP3_SDL_Build_Tests": self.tests_enabled,
        })
        return cmake

    def build(self):
        cmake = self._configed_cmake()
        cmake.build()

        if self.settings.os == "Emscripten":
            # TODO: Make this work.
            # May have to add
            # set(CMAKE_CXX_FLAGS "-s DISABLE_EXCEPTION_CATCHING=0")
            # to the root of CMakeLists.txt and turn on explicit support for
            # NodeFS in the test:
            # https://emscripten.org/docs/api_reference/Filesystem-API.html#filesystem-api-nodefs
            # cmd = "node {}/directory_tests".format(self.build_folder)
            # print(cmd)
            # self.run(cmd)
            pass
        elif not self.options.shared:
            # If SDL2 is shared, we won't be able to find it in most cases.
            cmake.test()


    def package(self):
        cmake = self._configed_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["lp3_rsrc"]
