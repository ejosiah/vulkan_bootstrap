from conans import ConanFile, CMake, tools


class VulkanUtilConan(ConanFile):
    name = "vulkanUtil"
    version = "0.1"
    license = "<Put the package license here>"
    author = "Josiah Ebhomenye joebhomenye@gmail.com"
    url = "https://github.com/ejosiah/vulkan_bootstrap"
    description = "cpp Vulkan wrapper"
    topics = ("Vulkan", "Graphics", "Physics")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "cmake"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def source(self):
        self.run("git clone https://github.com/ejosiah/vulkan_bootstrap.git")

    def build(self):
        tools
        cmake = CMake(self)
        cmake.configure(source_folder="vulkan_bootstrap")
        cmake.build()

        # Explicit way:
        # self.run('cmake %s/hello %s'
        #          % (self.source_folder, cmake.command_line))
        # self.run("cmake --build . %s" % cmake.build_config)

    def package(self):
        self.copy("*.h", dst="include", src="vulkan_bootstrap/3rdParty/include")
        self.copy("*.h", dst="include", src="vulkan_bootstrap/VulkanBase/include")
        self.copy("*.h", dst="include", src="vulkan_bootstrap/ImGuiPlugin/include")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["3rdParty.lib", "ImGuiPlugin.lib", "VulkanBase.lib"]

