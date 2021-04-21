from os import path
import os
from conans import ConanFile, CMake, tools

class GoogleTestConan(ConanFile):
    name = "gtest"
    version = "1.10.0"
    license = "gtest"
    url = "https://github.com/google/googletest.git"
    description = "google test"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    def _local_source_folder(self):
        return path.abspath(path.join(os.environ['THIRDPARTY_ROOT'], 'googletest'))


    def source(self):
        print('Nothing to do with sources. Using 3rdparty source: {}'.format(self._local_source_folder()))

    def _get_cmake(self):
        cmake = CMake(self)
        cmake.definitions['BUILD_GMOCK'] = True
        cmake.definitions['gtest_force_shared_crt'] = True
        cmake.configure(source_folder=self._local_source_folder())

        return cmake

    def build(self):
        self._get_cmake().build()

    def package(self):
        cmake = self._get_cmake()
        cmake.install()

    def package_info(self):
      build_type = self.settings.get_safe('build_type')
      if build_type == 'Debug':
        self.cpp_info.libs = ['gtestd', 'gtest_maind']
      else:
        self.cpp_info.libs = ['gtest', 'gtest_main']
