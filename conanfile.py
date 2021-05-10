from conans import ConanFile, CMake, tools
import os

class LibcoldConan(ConanFile):
    name = "libcold"
    version = "0.0.1"
    license = "Proprietary"
    author = "gviolator"
    url = "https://github.com/gviolator/libcold"
    description = "c++ library"
    topics = ("libcold", "gviolator", "conan-recipe")
    settings = {
        "compiler" : {
            "Visual Studio" : {
                "runtime" : ["MD", "MDd"],
                "version" : ["16"],
                "toolset" : ["v142"]
            },
            
            "clang" : {
                "version": ["11"]
            }
        },
        "build_type" : None,
        "arch" : None,
        "os" : ["Windows"]
    }
    options = {
        "shared": [True, False],
        "build_tests": [True, False]
    }
    default_options = {"shared": True, "build_tests": True}
    generators = "cmake_paths", "cmake_find_package", "cmake"
    # options = {"production": [True, False], "build_tests": [True, False]}
    # default_options = {"production": False, "build_tests": True} 
    exports_sources = "src*", "tests*", "cmake*", "extras*", "CMakeLists.txt",
    no_copy_source = True

    @property
    def os(self):
        return self.settings.get_safe('os')

    @property
    def compiler(self):
        return self.settings.get_safe('compiler')

    def build_requirements(self):
        # self.build_requires("cmake-toolkit/3.0.0@theice/production")
        pass

    def requirements(self):
        self.requires("libuv/1.41.0@libcold/local")
        self.options["libuv"].shared = True

        # self.requires("icu/64.2")
        # options["icu"].shared = True
        # self.options["icu"].data_packaging = 'library'

        # self.requires("rapidjson/cci.20200410")
        # build_tests = True

        # if (build_tests):
        #     self.requires("gtest/1.10.0@licold/local")

    @property
    def msbuild_options(self):
        return ["--", "/p:SolutionDir=" + self.build_folder + "/"]


    def _get_cmake(self):

        if self.os == 'Windows' and self.compiler == 'clang':
            cmake = CMake(self, generator = "Ninja", make_program = "c:/tools/ninja")
            cmake.definitions["CMAKE_TOOLCHAIN_FILE"] = os.path.join(self.build_folder, "conan_paths.cmake")
        else:
            cmake = CMake(self)

        if self.compiler == 'clang':
            if not "LLVM" in os.environ:
                raise Exception('LLVM env not set')

            llvm = os.environ['LLVM']

            cmake.definitions["CMAKE_C_COMPILER"] = 'e:/llvm/bin/clang'
            cmake.definitions["CMAKE_CXX_COMPILER"] = "e:/llvm/bin/clang++.exe"



        # return 

        # if self.os == 'Windows' and self.compiler == 'clang':
        #     cmake.definitions['CMAKE_EXPORT_COMPILE_COMMANDS'] = True
        #     cmake.generator = 'Ninja'

        cmake.definitions["CMAKE_TOOLCHAIN_FILE"] = os.path.join(self.build_folder, "conan_paths.cmake")

        cmake.definitions["VERSION"] = self.version if self.version else '0.0.1'
        # cmake.definitions["PRODUCTION"] = "1" if self.options.production else "0"
        cmake.definitions["CMAKE_MODULES_DIR"] = os.getenv('CMAKE_MODULES_DIR', 'cmake_modules')
        cmake.definitions["BUILDSCRIPTS_DIR"] = os.path.join(os.getenv('CMAKE_MODULES_DIR', ''), '..')
        cmake.definitions["CMAKE_SYSTEM_VERSION"] = "10.0.17763.0"

        # if self.os == 'Windows' and self.compiler == 'clang':
        #     cmake.definitions['CMAKE_EXPORT_COMPILE_COMMANDS'] = True
        #     cmake.generator = 'Ninja'
        
        if "PKG_EDITABLE_CONFIGURATION" in os.environ:
            cmake.definitions["PKG_EDITABLE_CONFIGURATION"] = True 
            self.options.build_tests = True

        if self.options.build_tests:
            cmake.definitions["ENABLE_TESTS"] = True

        print('CMD:')
        print(cmake.command_line)

        cmake.configure()

        return cmake

    def package(self):
        cmake = self._get_cmake()
        # cmake.build(args=self.msbuild_options)
        # cmake.install()

    def build(self):
        if self.should_configure:
            cmake = self._get_cmake()


        # if self.should_build:
        #     os.system('env_nuget && nuget restore ' + self.build_folder)
        #     cmake.build(args=self.msbuild_options)

    def imports(self):
        target_folder = os.path.join('target', self.settings.get_safe('build_type'))

        self.copy("icu*.dll", dst=target_folder, src="bin")
        self.copy("uv.dll", dst=target_folder, src="bin")
  
    def package_info(self):
        self.cpp_info.libs = ["libcold", "Rpcrt4"]
        if self.settings.compiler == "Visual Studio":
            self.cpp_info.cxxflags = ["-JMC", "-FC", "-Zc:__cplusplus", "-std:c++latest", "-permissive-"]
