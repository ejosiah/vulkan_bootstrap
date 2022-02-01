from conans import ConanFile, CMake, tools


class VmaConan(ConanFile):
    name = "vma"
    version = "0.0.1"
    license = "<Put the package license here>"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of Vma here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "cmake"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def source(self):
        self.run("git clone git@github.com:GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git")
        # This small hack might be useful to guarantee proper /MT /MD linkage
        # in MSVC if the packaged project doesn't have variables to set it
        # properly
        tools.replace_in_file("VulkanMemoryAllocator/CMakeLists.txt", "project(VulkanMemoryAllocator)",
                              '''project(VulkanMemoryAllocator)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()''')

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="VulkanMemoryAllocator")
        cmake.build()

        # Explicit way:
        # self.run('cmake %s/VulkanMemoryAllocator %s'
        #          % (self.source_folder, cmake.command_line))
        # self.run("cmake --build . %s" % cmake.build_config)

    def package(self):
        self.copy("include/vk_mem_alloc.h", dst="include/vma", src="VulkanMemoryAllocator", keep_path=False)
        self.copy("*.h", dst="include", src="VulkanMemoryAllocator", keep_path=True)
        self.copy("*vma.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["vma"]

