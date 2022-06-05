from conans import ConanFile, CMake, tools


class VulkanUtilConan(ConanFile):
    name = "vulkanUtil"
    version = "0.1.5"
    license = "<Put the package license here>"
    author = "Josiah Ebhomenye joebhomenye@gmail.com"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "vulkan utility library"
    topics = ("vulkan", "graphics", "3d")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "cmake"
    requires = ["assimp/5.0.1",
                "glm/0.9.9.8",
                "glfw/3.3.2",
                "stb/20200203",
                "spdlog/1.8.2",
                "freetype/2.10.4",
                "imgui/1.82",
                "boost/1.75.0",
                "gtest/1.11.0",
                "thrust/1.9.5",
                "argparse/2.1",
                "bullet3/3.17",
                "entt/3.8.1",
                "meshoptimizer/0.17",
                "zlib/1.2.12"
                ]

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def source(self):
        self.run("git clone https://github.com/ejosiah/vulkan_bootstrap.git")
        self.run("git -C %s/vulkan_bootstrap checkout v%s" % (self.source_folder, self.version))

    def build(self):
        self.run('conan install %s/vulkan_bootstrap -s build_type=%s' % (self.source_folder, self.settings.build_type.value))
        cmake = CMake(self)
        cmake.configure(source_folder="vulkan_bootstrap")
        # cmake.build()

        # Explicit way:
        self.run('cmake %s/vulkan_bootstrap %s'
                 % (self.source_folder, cmake.command_line))
        self.run("cmake %s/vulkan_bootstrap -D BUILD_EXAMPLES:BOOL=OFF" % self.source_folder)
        self.run("cmake --build . %s" % cmake.build_config)

    def package(self):
        self.copy("VulkanBase/*.h*", dst="include/vulkan_util", src="vulkan_bootstrap", keep_path=False)
        self.copy("3rdParty/*.h*", dst="include/vulkan_util", src="vulkan_bootstrap", keep_path=False)
        self.copy("ImGuiPlugin/*.h*", dst="include/vulkan_util", src="vulkan_bootstrap", keep_path=False)
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["VulkanBase", "ImGuiPlugin"]

