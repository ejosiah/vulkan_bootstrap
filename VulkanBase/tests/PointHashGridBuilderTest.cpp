#include "VulkanFixture.hpp"

constexpr int ITEMS_PER_WORKGROUP = 8 << 10;

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
    struct {
        VulkanDescriptorSetLayout gridDescriptorSetLayout;
        VulkanDescriptorSetLayout bucketSizeSetLayout;
        VulkanDescriptorSetLayout particleDescriptorSetLayout;
        VkDescriptorSet gridDescriptorSet;
        VkDescriptorSet bucketSizeDescriptorSet;
        VkDescriptorSet bucketSizeOffsetDescriptorSet;
        VkDescriptorSet particleDescriptorSet;
        VulkanBuffer bucketSizeBuffer;
        VulkanBuffer bucketSizeOffsetBuffer;
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

    struct {
        VkDescriptorSet descriptorSet;
        VkDescriptorSet sumScanDescriptorSet;
        VulkanDescriptorSetLayout setLayout;
        VulkanBuffer sumsBuffer;
        struct {
            int itemsPerWorkGroup = 8 << 10;
            int N = 0;
        } constants;
    } prefixScan;

    const uint32_t  particlePushConstantsSize = 20;
    std::vector<Particle> particles;
    std::map<int, std::vector<Particle>> expectedHashGrid;
    uint32_t pushConstantOffset = alignedSize(sizeof(Camera) + particlePushConstantsSize, 16);
    uint32_t bufferOffsetAlignment;


    void SetUp() final {
        VulkanFixture::SetUp();
//        spdlog::set_level(spdlog::level::off);
    }

    void postVulkanInit() override {
        bufferOffsetAlignment = device.getLimits().minStorageBufferOffsetAlignment;
        createDescriptorSets();
        particles.clear();
        expectedHashGrid.clear();
    }

    std::vector<PipelineMetaData> pipelineMetaData() override {
        createDescriptorSetLayouts();
        createPrefixScanDescriptorSetLayouts();
        return {
                {
                    "point_hash_grid_builder",
                    "../../data/shaders/sph/point_hash_grid_builder.comp.spv",
                    { &gridBuilder.particleDescriptorSetLayout, &gridBuilder.gridDescriptorSetLayout, &gridBuilder.bucketSizeSetLayout},
                    {{VK_SHADER_STAGE_COMPUTE_BIT, pushConstantOffset, sizeof(gridBuilder.constants)}}
                },
                {
                        "prefix_scan",
                        "../../data/shaders/sph/scan.comp.spv",
                        { &prefixScan.setLayout }
                },
                {
                        "add",
                        "../../data/shaders/sph/add.comp.spv",
                        { &prefixScan.setLayout },
                        { { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(prefixScan.constants)} }
                }
        };
    }

    void createDescriptorSetLayouts(){
        std::vector<VkDescriptorSetLayoutBinding> bindings(1);
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        gridBuilder.particleDescriptorSetLayout = device.createDescriptorSetLayout(bindings);

        bindings.resize(2);
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        gridBuilder.gridDescriptorSetLayout = device.createDescriptorSetLayout(bindings);

        bindings.resize(1);
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        gridBuilder.bucketSizeSetLayout = device.createDescriptorSetLayout(bindings);
    }

    void createPrefixScanDescriptorSetLayouts(){
        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        prefixScan.setLayout = device.createDescriptorSetLayout(bindings);
    }

    void createDescriptorSets(){
        auto sets = descriptorPool.allocate({ gridBuilder.particleDescriptorSetLayout,
                                              gridBuilder.gridDescriptorSetLayout,
                                              prefixScan.setLayout, prefixScan.setLayout, gridBuilder.bucketSizeSetLayout, gridBuilder.bucketSizeSetLayout } );
        gridBuilder.particleDescriptorSet = sets[0];
        gridBuilder.gridDescriptorSet = sets[1];
        prefixScan.descriptorSet = sets[2];
        prefixScan.sumScanDescriptorSet = sets[3];
        gridBuilder.bucketSizeDescriptorSet = sets[4];
        gridBuilder.bucketSizeOffsetDescriptorSet = sets[5];
    }

    void updateDescriptorSet(){
        auto writes = initializers::writeDescriptorSets<4>();

        // particle descriptor set
        VkDescriptorBufferInfo info0{ gridBuilder.particleBuffer, 0, VK_WHOLE_SIZE};
        writes[0].dstSet = gridBuilder.particleDescriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].pBufferInfo = &info0;

        // grid descriptor set
        VkDescriptorBufferInfo  nextBucketIndexInfo{ gridBuilder.nextBufferIndexBuffer, 0, VK_WHOLE_SIZE };
        writes[1].dstSet = gridBuilder.gridDescriptorSet;
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].pBufferInfo = &nextBucketIndexInfo;

        // bucket size / offset
        VkDescriptorBufferInfo  bucketSizeInfo{ gridBuilder.bucketSizeBuffer, 0, VK_WHOLE_SIZE };
        writes[2].dstSet = gridBuilder.bucketSizeDescriptorSet;
        writes[2].dstBinding = 0;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].pBufferInfo = &bucketSizeInfo;

        VkDescriptorBufferInfo  bucketOffsetInfo{ gridBuilder.bucketSizeOffsetBuffer, 0, VK_WHOLE_SIZE };
        writes[3].dstSet = gridBuilder.bucketSizeOffsetDescriptorSet;
        writes[3].dstBinding = 0;
        writes[3].descriptorCount = 1;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[3].pBufferInfo = &bucketOffsetInfo;


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

    void updateGridBuffer(VkDeviceSize bucketSize = 0){
        auto res = gridBuilder.constants.resolution;
        VkDeviceSize gridSize = res.x * res.y * res.z * sizeof(uint32_t);

        if(bucketSize == 0){
            gridBuilder.nextBufferIndexBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, gridSize);
            gridBuilder.bucketSizeBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, gridSize);
            gridBuilder.bucketSizeOffsetBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, gridSize);
            gridBuilder.bucketBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,sizeof(uint32_t));
            updateScanBuffer();
            execute([&](auto commandBuffer){
                vkCmdFillBuffer(commandBuffer, gridBuilder.nextBufferIndexBuffer, 0, gridSize, 0);
                vkCmdFillBuffer(commandBuffer, gridBuilder.bucketSizeBuffer, 0, gridSize, 0);
            });
        }else{
            gridBuilder.bucketBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, bucketSize);
        }
    }

    void updateScanBuffer(){
        VkDeviceSize sumsSize = (std::abs(int(particles.size() - 1))/ITEMS_PER_WORKGROUP + 1) * sizeof(int);
        sumsSize = alignedSize(sumsSize, bufferOffsetAlignment) + sizeof(int);
        prefixScan.sumsBuffer = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sumsSize);
        prefixScan.constants.N = particles.size()/sizeof(int);
        updateScanDescriptorSet();
    }

    void updateScanDescriptorSet(){
        VkDescriptorBufferInfo info{ gridBuilder.bucketSizeOffsetBuffer, 0, VK_WHOLE_SIZE};
        auto writes = initializers::writeDescriptorSets<4>(prefixScan.descriptorSet);
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].pBufferInfo = &info;

        VkDescriptorBufferInfo sumsInfo{ prefixScan.sumsBuffer, 0, prefixScan.sumsBuffer.size - sizeof(int)};
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].pBufferInfo = &sumsInfo;

        // for sum scan
        writes[2].dstSet = prefixScan.sumScanDescriptorSet;
        writes[2].dstBinding = 0;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].pBufferInfo = &sumsInfo;

        VkDescriptorBufferInfo sumsSumInfo{ prefixScan.sumsBuffer, prefixScan.sumsBuffer.size - sizeof(int), VK_WHOLE_SIZE}; // TODO FIXME
        writes[3].dstSet = prefixScan.sumScanDescriptorSet;
        writes[3].dstBinding = 1;
        writes[3].descriptorCount = 1;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[3].pBufferInfo = &sumsSumInfo;

        device.updateDescriptorSets(writes);
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

    void scan(VkCommandBuffer commandBuffer){
        int size = particles.size();
        int numWorkGroups = std::abs(size - 1)/ITEMS_PER_WORKGROUP + 1;
        addBufferMemoryBarriers(commandBuffer, {&gridBuilder.bucketSizeBuffer});  // make sure grid build for pass 0 finished
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("prefix_scan"));
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("prefix_scan"), 0, 1, &prefixScan.descriptorSet, 0, nullptr);
        vkCmdDispatch(commandBuffer, numWorkGroups, 1, 1);

        if(numWorkGroups > 1){
            addBufferMemoryBarriers(commandBuffer, {&gridBuilder.bucketSizeOffsetBuffer, &prefixScan.sumsBuffer});
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("prefix_scan"), 0, 1, &prefixScan.sumScanDescriptorSet, 0, nullptr);
            vkCmdDispatch(commandBuffer, 1, 1, 1);

            addBufferMemoryBarriers(commandBuffer, { &gridBuilder.bucketSizeOffsetBuffer, &prefixScan.sumsBuffer });
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("add"));
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("add"), 0, 1, &prefixScan.descriptorSet, 0, nullptr);
            vkCmdPushConstants(commandBuffer, layout("add"), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(prefixScan.constants), &prefixScan.constants);
            vkCmdDispatch(commandBuffer, numWorkGroups, 1, 1);
        }
        // make sure bucketSize before write finished before grid build pass 1
        addBufferMemoryBarriers(commandBuffer, {&gridBuilder.bucketSizeOffsetBuffer});
    }

    void addBufferMemoryBarriers(VkCommandBuffer commandBuffer, const std::vector<VulkanBuffer*>& buffers){
        std::vector<VkBufferMemoryBarrier> barriers(buffers.size());

        for(int i = 0; i < buffers.size(); i++) {
            barriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barriers[i].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barriers[i].offset = 0;
            barriers[i].buffer = *buffers[i];
            barriers[i].size = buffers[i]->size;
        }

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                             nullptr, COUNT(barriers), barriers.data(), 0, nullptr);

    }

    void buildHashGrid(){
        updateGridBuffer();
        updateDescriptorSet();
        generateHashGrid(0);
        updateBucketBuffers();
        generateHashGrid(1);
    }

    void updateBucketBuffers(){
        VkDeviceSize size = prefixScan.sumsBuffer.get<int>(0) * sizeof(int);
        updateGridBuffer(size * sizeof(int));
        updateBucketDescriptor();
    }

    void generateHashGrid(int pass){
        execute([&](auto commandBuffer){
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("point_hash_grid_builder"));

            gridBuilder.constants.pass = pass;
            vkCmdPushConstants(commandBuffer, layout("point_hash_grid_builder"), VK_SHADER_STAGE_COMPUTE_BIT, pushConstantOffset,
                               sizeof(gridBuilder.constants), &gridBuilder.constants);

            static std::array<VkDescriptorSet, 3> sets;
            sets[0] = gridBuilder.particleDescriptorSet;
            sets[1] = gridBuilder.gridDescriptorSet;
            sets[2] = (pass%2) ? gridBuilder.bucketSizeOffsetDescriptorSet : gridBuilder.bucketSizeDescriptorSet;

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("point_hash_grid_builder"), 0, COUNT(sets), sets.data(), 0, nullptr);
            vkCmdDispatch(commandBuffer, (particles.size() - 1)/1024 + 1, 1, 1);


            // if pass 0
            // copy bucketSize into bucketSizeOffset
            // scan bucketSizeOffset
            // size of bucket buffer should be available after sum
            if(pass == 0){
                VkBufferCopy region{0, 0, gridBuilder.bucketSizeBuffer.size};
                vkCmdCopyBuffer(commandBuffer, gridBuilder.bucketSizeBuffer, gridBuilder.bucketSizeOffsetBuffer, 1, &region);
                scan(commandBuffer);
            }
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
};

TEST_F(PointHashGridBuilderTest, OnePointPerGrid2d){
    float gridSpacing = 0.1f;
    glm::vec3 resolution{10, 10, 1};
    auto generateParticle = [&](int x, int y, int _) -> std::vector<Particle> {
        Particle particle;
        glm::vec3 pos = (glm::vec3(x, y, 0) + glm::vec3(0.5f, 0.5f, 0)) * gridSpacing;
        particle.position = glm::vec4(pos, 1);
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
        glm::vec3 pos = (glm::vec3(x, y, z) + 0.5f) * gridSpacing;
        particle.position = glm::vec4(pos, 0); // range [0, 1]
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
        glm::vec3 pos = (glm::vec3(x, y, z) + 0.5f) * gridSpacing - 1.0f;
        particle.position = glm::vec4(pos, 1); // [-1, 1]
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