from os import path
import os
from conans import ConanFile, CMake, tools

class LibuvConan(ConanFile):
    name = "libuv"
    version = "1.41.0"
    license = "libuv"
    url = "https://github.com/libuv/libuv"
    description = "libuv"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": True}
    generators = "cmake"

    @property
    def os(self):
        return self.settings.get_safe('os')

    @property
    def compiler(self):
        return self.settings.get_safe('compiler')

    def source(self):
        src = path.abspath(path.join(os.environ['THIRDPARTY_ROOT'], 'libuv'))
        print('Nothing to do with sources. Using 3rdparty source: {}'.format(src))

    def _get_cmake(self):
        src = path.abspath(path.join(os.environ['THIRDPARTY_ROOT'], 'libuv'))

        cmake = CMake(self)
        cmake.definitions['LIBUV_BUILD_TESTS'] = False
        cmake.definitions['LIBUV_BUILD_BENCH'] = False
        if self.os == 'Windows' and self.compiler == 'clang':
            cmake.definitions['CMAKE_EXPORT_COMPILE_COMMANDS'] = True
            cmake.generator = 'Ninja'

        cmake.configure(source_folder=src)

        

        return cmake



    def build(self):
        self._get_cmake().build()

    def package(self):

        cmake = self._get_cmake()
        cmake.install()

        build_type = self.settings.get_safe('build_type')
        packageLibPath = path.join(self.package_folder, 'lib')
        packageLibBuildPath = path.join(packageLibPath, build_type)

        os_name = self.settings.get_safe('os')

        if os_name == 'Windows':
            print ('Copying libs on Windows')

            os.renames(path.join(packageLibBuildPath, 'uv.dll'), path.join(self.package_folder, 'bin', 'uv.dll'))
            os.replace(path.join(packageLibBuildPath, 'uv.lib'), path.join(packageLibPath, 'uv.lib'))
            os.replace(path.join(packageLibBuildPath, 'uv_a.lib'), path.join(packageLibPath, 'uv_a.lib'))
        elif os_name == 'Linux':
            print ('Copying libs on Linux')


    def package_info(self):
        if self.options.shared:
            self.cpp_info.libs = ['uv']
        else:
            self.cpp_info.libs = ['uv_a']