#include "VulkanFixture.hpp"

struct Particle{
    glm::vec4 position{0};
    glm::vec4 color{0};
    glm::vec3 velocity{0};
    float invMass{0};
};

inline bool similar(const Particle& a, const Particle& b, float epsilon = 1E-7){
    return closeEnough(a.position.x, b.position.x, epsilon)
           && closeEnough(a.position.y, b.position.y, epsilon)
           && closeEnough(a.position.z, b.position.z, epsilon);
};

class PointHashGridBuilderTest : public VulkanFixture{
protected:

    void SetUp() final {
        VulkanFixture::SetUp();
        spdlog::set_level(spdlog::level::off);
        createDescriptorPool();
        createDescriptorSetLayouts();
        createDescriptorSets();
        createPipeline();

        particles.clear();
        expectedHashGrid.clear();
    }

    void createDescriptorPool(){
        std::vector<VkDescriptorPoolSize> poolSizes{
                {
                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10}
                }
        };
        descriptorPool = device.createDescriptorPool(2, poolSizes);
    }

    void createDescriptorSetLayouts(){
        std::vector<VkDescriptorSetLayoutBinding> bindings(1);
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        gridBuilder.particleDescriptorSetLayout = device.createDescriptorSetLayout(bindings);

        bindings.resize(3);
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[2].binding = 2;
        bindings[2].descriptorCount = 1;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        gridBuilder.gridDescriptorSetLayout = device.createDescriptorSetLayout(bindings);
    }

    void createDescriptorSets(){
        auto sets = descriptorPool.allocate({ gridBuilder.particleDescriptorSetLayout, gridBuilder.gridDescriptorSetLayout } );
        gridBuilder.particleDescriptorSet = sets.front();
        gridBuilder.gridDescriptorSet = sets.back();
    }

    void updateDescriptorSet(){
        auto writes = initializers::writeDescriptorSets<3>();

        // particle descriptor set
        VkDescriptorBufferInfo info0{ gridBuilder.particleBuffer, 0, VK_WHOLE_SIZE};
        writes[0].dstSet = gridBuilder.particleDescriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].pBufferInfo = &info0;

        // grid descriptor set
        VkDescriptorBufferInfo  bucketSizeInfo{ gridBuilder.bucketSizeBuffer, 0, VK_WHOLE_SIZE };
        writes[1].dstSet = gridBuilder.gridDescriptorSet;
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].pBufferInfo = &bucketSizeInfo;

        VkDescriptorBufferInfo  nextBucketIndexInfo{ gridBuilder.nextBufferIndexBuffer, 0, VK_WHOLE_SIZE };
        writes[2].dstSet = gridBuilder.gridDescriptorSet;
        writes[2].dstBinding = 2;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].pBufferInfo = &nextBucketIndexInfo;
        device.updateDescriptorSets(writes);
        updateBucketDescriptor();
    }

    void updateBucketDescriptor(){
        auto writes = initializers::writeDescriptorSets<1>();
        VkDescriptorBufferInfo bucketInfo{ gridBuilder.bucketBuffer, 0, VK_WHOLE_SIZE };
        writes[0].dstSet = gridBuilder.gridDescriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].pBufferInfo = &bucketInfo;

        device.updateDescriptorSets(writes);
    }

    void createPipeline() {
        auto gridBuilderModule = VulkanShaderModule{"../../data/shaders/sph/point_hash_grid_builder.comp.spv", device};
        auto stage = initializers::shaderStage({gridBuilderModule, VK_SHADER_STAGE_COMPUTE_BIT});
        std::vector<VkPushConstantRange> ranges{{VK_SHADER_STAGE_COMPUTE_BIT, pushConstantOffset, sizeof(gridBuilder.constants)}};
        gridBuilder.layout = device.createPipelineLayout({ gridBuilder.particleDescriptorSetLayout, gridBuilder.gridDescriptorSetLayout }, ranges);

        auto createInfo = initializers::computePipelineCreateInfo();
        createInfo.stage = stage;
        createInfo.layout = gridBuilder.layout;

        gridBuilder.pipeline = device.createComputePipeline(createInfo);
    }

    void updateGridBuffer(VkDeviceSize bucketSize = 0){
        auto res = gridBuilder.constants.resolution;
        VkDeviceSize gridSize = res.x * res.y * res.z * sizeof(uint32_t);

        if(bucketSize == 0){
            gridBuilder.nextBufferIndexBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, gridSize);
            gridBuilder.bucketSizeBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, gridSize);
            gridBuilder.bucketBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,sizeof(uint32_t));

            execute([&](auto commandBuffer){
                vkCmdFillBuffer(commandBuffer, gridBuilder.nextBufferIndexBuffer, 0, gridSize, 0);
                vkCmdFillBuffer(commandBuffer, gridBuilder.bucketSizeBuffer, 0, gridSize, 0);
            });
        }else{
            gridBuilder.bucketBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, bucketSize);
        }
    }

    void createParticleBuffer(const std::vector<Particle> particles){
        gridBuilder.particleBuffer = device.createCpuVisibleBuffer(particles.data(), sizeof(Particle) * particles.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }


    template<typename Generate, typename Hash>
    void createParticles(Generate&& generate, Hash&& hash){
        auto res = gridBuilder.constants.resolution;
        auto gridSpacing = gridBuilder.constants.gridSpacing;
        for(int z = 0; z < res.z; z++){
            for(int y = 0; y < res.y; y++){
                for(int x = 0; x < res.x; x++){
                    std::vector<Particle> particles = generate(x, y, z);
                    int index = hash(particles.front().position.xyz(), res, gridSpacing);
                    add(index, particles);
                }
            }
        }
        createParticleBuffer(this->particles);
        gridBuilder.particleBuffer.map<Particle>([&](auto ptr){
           for(int i = 0; i < this->particles.size(); i++){
           }
        });
        gridBuilder.constants.numParticles = particles.size();
    }

    void add(int index, const std::vector<Particle>& vParticles){
        if(expectedHashGrid.find(index) == end(expectedHashGrid)){
            expectedHashGrid[index] = std::vector<Particle>{};
        }
        expectedHashGrid[index].insert(begin(expectedHashGrid[index]), begin(vParticles), end(vParticles));
        particles.insert(begin(particles), begin(vParticles), end(vParticles));
    }

    bool bucketContainsPoint(int key, const Particle& particle){
        bool contains = false;
        gridBuilder.bucketBuffer.map<int>([&](auto bucketPtr){
            gridBuilder.bucketSizeBuffer.map<int>([&](auto bucketSizePtr){
                gridBuilder.particleBuffer.map<Particle>([&](auto particlePtr){

                    auto getOffset = [&](){
                        int offset = 0;
                        for(int i = key - 1; i >= 0; i--){
                            offset += bucketSizePtr[i];
                        };
                        return offset;
                    };

                    auto numParticles = bucketSizePtr[key];
                    for(int i = 0; i < numParticles; i++){
                        int particleId = bucketPtr[getOffset() + i];
                        Particle bParticle = particlePtr[particleId];
                        contains = similar(particle, bParticle);
                        if(contains){
                            break;
                        }
                    }
                });
            });
        });
        return contains;
    }

    void buildHashGrid(){
        updateGridBuffer();
        updateDescriptorSet();
        generateHashGrid(0);
        updateBucketBuffers();
        generateHashGrid(1);
    }

    void updateBucketBuffers(){
        VkDeviceSize size = 0;
        auto res = gridBuilder.constants.resolution;
        gridBuilder.bucketSizeBuffer.map<int>([&](auto itr){
            VkDeviceSize end =  res.x * res.y * res.z;
            for(int i = 0; i < end; i++ ){
                auto bucketSize = *(itr + i);
                size += bucketSize;
                spdlog::error("bucket: {}, size: {}", i, bucketSize);
            }
        });
        updateGridBuffer(size * sizeof(int));
        updateBucketDescriptor();
    }

    void generateHashGrid(int pass){
        execute([&](auto commandBuffer){
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, gridBuilder.pipeline);

            gridBuilder.constants.pass = pass;
            vkCmdPushConstants(commandBuffer, gridBuilder.layout, VK_SHADER_STAGE_COMPUTE_BIT, pushConstantOffset,
                               sizeof(gridBuilder.constants), &gridBuilder.constants);

            static std::array<VkDescriptorSet, 2> sets;
            sets[0] = gridBuilder.particleDescriptorSet;
            sets[1] = gridBuilder.gridDescriptorSet;

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, gridBuilder.layout, 0, COUNT(sets), sets.data(), 0, nullptr);
            vkCmdDispatch(commandBuffer, particles.size()/1024 + 1, 1, 1);
        });
    }

    void AssertGrid(){
        ASSERT_FALSE(particles.empty()) << "Empty grid, you need to generate particles first";
        for(const auto& [bucket, particles] : expectedHashGrid){
            for(const auto& expected : particles) {
                ASSERT_TRUE(bucketContainsPoint(bucket, expected))
                    << fmt::format("expected {} not found in bucket : {}", expected.position, bucket);
            }
        }
    }

    static int defaultHash(glm::vec3 point, glm::vec3 size, float gridSpacing){
        glm::vec3 bucketIndex = glm::floor(point/gridSpacing);
        bucketIndex = mod(bucketIndex, size);
        bucketIndex += (1.0f - glm::step(0.0f, bucketIndex)) * size;

        return int(bucketIndex.z * size.y + bucketIndex.y) * size.x + bucketIndex.x;
    }

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanDescriptorSetLayout gridDescriptorSetLayout;
        VulkanDescriptorSetLayout particleDescriptorSetLayout;
        VkDescriptorSet gridDescriptorSet;
        VkDescriptorSet particleDescriptorSet;
        VulkanBuffer bucketSizeBuffer;
        VulkanBuffer bucketBuffer;
        VulkanBuffer particleBuffer;
        VulkanBuffer nextBufferIndexBuffer;

        struct {
            glm::vec3 resolution{1};
            float gridSpacing{1};
            uint32_t pass{0};
            uint32_t numParticles{0};
        } constants;
    } gridBuilder;

    const uint32_t  particlePushConstantsSize = 20;
    std::vector<Particle> particles;
    std::map<int, std::vector<Particle>> expectedHashGrid;
    VulkanDescriptorPool descriptorPool;
    uint32_t pushConstantOffset = alignedSize(sizeof(Camera) + particlePushConstantsSize, 16);
};

TEST_F(PointHashGridBuilderTest, OnePointPerGrid2d){
    float gridSpacing = 0.1f;
    glm::vec3 resolution{10, 10, 1};
    auto generateParticle = [&](int x, int y, int _) -> std::vector<Particle> {
        Particle particle;
        particle.position = glm::vec4(glm::vec3(float(x) + 0.5f) * gridSpacing, 1);
        return { particle };
    };
    gridBuilder.constants.resolution = resolution;
    gridBuilder.constants.gridSpacing = gridSpacing;

    createParticles(generateParticle, defaultHash);
    buildHashGrid();
    AssertGrid();
}

TEST_F(PointHashGridBuilderTest, OnePointPerGrid3d){
    float gridSpacing{0.1};
    glm::vec3 resolution{10, 10, 10};
    auto generateParticle = [&](int x, int y, int z) -> std::vector<Particle> {

        Particle particle;
        particle.position = glm::vec4(glm::vec3(float(x) + 0.5f) * gridSpacing, 0); // range [0, 1]
        return { particle };
    };
    gridBuilder.constants.resolution = resolution;
    gridBuilder.constants.gridSpacing = gridSpacing;

    createParticles(generateParticle, defaultHash);
    buildHashGrid();
    AssertGrid();
}

TEST_F(PointHashGridBuilderTest, generateGridWithPointsInNegativeAndPositiveSpace){
    float gridSpacing{0.2};
    glm::vec3 resolution{4, 4, 1};
    auto generateParticle = [&](int x, int y, int z) -> std::vector<Particle> {
        Particle particle;
        particle.position = glm::vec4(glm::vec3(float(x) + 0.5f) * gridSpacing - 1.0f, 1); // [-1, 1]
        return { particle };
    };
    gridBuilder.constants.resolution = resolution;
    gridBuilder.constants.gridSpacing = gridSpacing;

    createParticles(generateParticle, defaultHash);
    buildHashGrid();
    AssertGrid();
}

TEST_F(PointHashGridBuilderTest, PointRandomlyScatteredInSpace){
//    std::default_random_engine engine{ 1 << 20};
//    std::uniform_real_distribution<float> dist(0, 0.9);
    auto rng = rngFunc<float>(0, 0.9, 1 << 20);
    glm::vec3 resolution{4, 4, 4};
    float gridSpacing = 0.25;

    auto generateParticle = [&](int x, int y, int z) -> std::vector<Particle> {

        Particle particle;
        particle.position.x = rng();
        particle.position.y = rng();
        particle.position.z = rng();
        return { particle };
    };
    gridBuilder.constants.resolution = resolution;
    gridBuilder.constants.gridSpacing = gridSpacing;

    createParticles(generateParticle, defaultHash);
    buildHashGrid();
    AssertGrid();
}