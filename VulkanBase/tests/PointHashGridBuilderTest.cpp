#include "VulkanFixture.hpp"
#include "PointHashGrid.hpp"
#include "sampling.hpp"

inline bool similar(const glm::vec4& a, const glm::vec4& b, float epsilon = 1E-7){
    return closeEnough(a.x, b.x, epsilon)
           && closeEnough(a.y, b.y, epsilon)
           && closeEnough(a.z, b.z, epsilon);
};

inline bool similar(const glm::vec3& a, const glm::vec3& b, float epsilon = 1E-7){
    return closeEnough(a.x, b.x, epsilon)
           && closeEnough(a.y, b.y, epsilon)
           && closeEnough(a.z, b.z, epsilon);
};

class PointHashGridBuilderTest : public VulkanFixture{
protected:
    VulkanBuffer particleBuffer;
    VulkanDescriptorSetLayout particleDescriptorSetLayout{};
    VulkanDescriptorSetLayout unitTestDescriptorSetLayout{};
    VkDescriptorSet particleDescriptorSet;
    VkDescriptorSet unitTestDescriptorSet;
    VulkanBuffer nearByKeys{};
    PointHashGrid grid;
    glm::vec3 resolution{1};
    float gridSpacing = 1;

    const uint32_t  particlePushConstantsSize = 20;
    std::vector<glm::vec4> particles;
    std::map<int, std::vector<glm::vec4>> expectedHashGrid;
    uint32_t pushConstantOffset = alignedSize(sizeof(Camera) + particlePushConstantsSize, 16);
    uint32_t bufferOffsetAlignment;


    void SetUp() final {
        VulkanFixture::SetUp();
//        spdlog::set_level(spdlog::level::off);
    }

    std::vector<PipelineMetaData> pipelineMetaData() override {
        return
        {
                {
                        "point_hash_grid_unit_test",
                        "../../data/shaders/test/point_hash_grid_test.comp.spv",
                        { &unitTestDescriptorSetLayout},
                        {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(grid.constants)}}
                }
        };
    }

    void postVulkanInit() override {
        bufferOffsetAlignment = device.getLimits().minStorageBufferOffsetAlignment;
        particles.clear();
        expectedHashGrid.clear();
        createDescriptorSetLayouts();
    }

    void createDescriptorSetLayouts(){
        std::vector<VkDescriptorSetLayoutBinding> bindings(1);
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        particleDescriptorSetLayout = device.createDescriptorSetLayout(bindings);

        // unit test layout
        bindings.resize(2);
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        unitTestDescriptorSetLayout = device.createDescriptorSetLayout(bindings);
    }

    void createParticleBuffer(const std::vector<glm::vec4>& particles){
        particleBuffer = device.createCpuVisibleBuffer(particles.data(), sizeof(glm::vec4) * particles.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        nearByKeys = device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(int) * 8);
        updateDescriptorSet();
    }

    void updateDescriptorSet(){
        auto sets = descriptorPool.allocate({particleDescriptorSetLayout, unitTestDescriptorSetLayout});
        particleDescriptorSet = sets[0];
        unitTestDescriptorSet = sets[1];

        auto writes = initializers::writeDescriptorSets<3>();

        // particle descriptor set
        VkDescriptorBufferInfo info0{particleBuffer, 0, VK_WHOLE_SIZE};
        writes[0].dstSet = particleDescriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].pBufferInfo = &info0;

        // unit test writes
        writes[1].dstSet = unitTestDescriptorSet;
        writes[1].dstBinding = 0;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].pBufferInfo = &info0;

        VkDescriptorBufferInfo  nearByKeysInfo{nearByKeys, 0, VK_WHOLE_SIZE };
        writes[2].dstSet = unitTestDescriptorSet;
        writes[2].dstBinding = 1;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].pBufferInfo = &nearByKeysInfo;

        device.updateDescriptorSets(writes);
    }


    template<typename Generate, typename Hash>
    void createParticles(Generate&& generate, Hash&& hash){
        auto res = grid.constants.resolution;
        auto gridSpacing = grid.constants.gridSpacing;
        for(int z = 0; z < res.z; z++){
            for(int y = 0; y < res.y; y++){
                for(int x = 0; x < res.x; x++){
                    std::vector<glm::vec4> particles = generate(x, y, z);
                    int index = hash(particles.front().xyz(), res, gridSpacing);
                    add(index, particles);
                }
            }
        }
        createParticleBuffer(this->particles);
        grid.constants.numParticles = particles.size();
    }

    void createParticles(){
        createParticleBuffer(this->particles);
    }

    void add(int index, const std::vector<glm::vec4>& vParticles){
        if(expectedHashGrid.find(index) == end(expectedHashGrid)){
            expectedHashGrid[index] = std::vector<glm::vec4>{};
        }
        expectedHashGrid[index].insert(begin(expectedHashGrid[index]), begin(vParticles), end(vParticles));
        particles.insert(begin(particles), begin(vParticles), end(vParticles));
    }

    bool bucketContainsPoint(int key, const glm::vec4& particle){
        bool contains = false;
        grid.bucketBuffer.map<int>([&](auto bucketPtr){
            grid.bucketSizeBuffer.map<int>([&](auto bucketSizePtr){
                particleBuffer.map<glm::vec4>([&](auto particlePtr){

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
                        glm::vec4 bParticle = particlePtr[particleId];
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
        grid.scan(commandBuffer);
    }


    void buildHashGrid(){
        grid = PointHashGrid{&device, &descriptorPool, &particleDescriptorSetLayout, int(particles.size()), resolution, gridSpacing};
        execute([&](auto commandBuffer){
            grid.buildHashGrid(commandBuffer, particleDescriptorSet);
        });
    }

    void AssertGrid(){
        ASSERT_FALSE(particles.empty()) << "Empty grid, you need to generate particles first";
        for(const auto& [bucket, particles] : expectedHashGrid){
            for(const auto& expected : particles) {
                ASSERT_TRUE(bucketContainsPoint(bucket, expected))
                    << fmt::format("expected {} not found in bucket : {}", expected, bucket);
            }
        }
    }

    static int defaultHash(glm::vec3 point, glm::vec3 size, float gridSpacing){
        glm::vec3 bucketIndex = glm::floor(point/gridSpacing);
        bucketIndex = mod(bucketIndex, size);
        bucketIndex += (1.0f - glm::step(0.0f, bucketIndex)) * size;

        return int(bucketIndex.z * size.y + bucketIndex.y) * size.x + bucketIndex.x;
    }

    std::function<bool(int, int, int, int)> containsNearbyKey = [&](int key0, int key1, int key2, int key3){
        std::array<bool, 4> predicates{};
        nearByKeys.map<int>([&](auto ptr){
            for(int i = 0; i < 4; i++){
                if(key0 == ptr[i]){
                    predicates[i] = true;
                }
                if(key1 == ptr[i]){
                    predicates[i] = true;
                }
                if(key2 == ptr[i]){
                    predicates[i] = true;
                }
                if(key3 == ptr[i]){
                    predicates[i] = true;
                }
            }
        });
        return std::all_of(begin(predicates), end(predicates), [](auto flag){ return flag; });
    };

    void addParticleAt(glm::vec3 position){
        glm::vec4 particle;
        particle = glm::vec4(position, 0);
        particles.push_back(particle);
    }

    void addParticleAt(float x, float y, float z = 0){
        addParticleAt({x, y, z});
    }

    void getNearByKeys(){
        execute([&](auto commandBuffer){
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline("point_hash_grid_unit_test"));
            vkCmdPushConstants(commandBuffer, layout("point_hash_grid_unit_test"), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                               sizeof(grid.constants), &grid.constants);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout("point_hash_grid_unit_test"), 0, 1, &unitTestDescriptorSet, 0, nullptr);
            vkCmdDispatch(commandBuffer, 1, 1, 1);

        });
    }

    void generateNeighbourList(){
        grid = PointHashGrid{&device, &descriptorPool, &particleDescriptorSetLayout, int(particles.size()), resolution, gridSpacing};
        execute([&](auto commandBuffer){
            grid.buildHashGrid(commandBuffer, particleDescriptorSet);
            grid.generateNeighbourList(commandBuffer);
        });
    }

    void logGrid(){
        grid.bucketBuffer.map<int>([&](auto bucketPtr){
            grid.bucketSizeOffsetBuffer.map<int>([&](auto bucketOffsetPtr){
            grid.bucketSizeBuffer.map<int>([&](auto bucketSizePtr){
                particleBuffer.map<glm::vec4>([&](auto particlePtr){
                    int numBuckets = resolution.x * resolution.y * resolution.y;
                    for(int bucket = 0; bucket < numBuckets; bucket++){
                        int size = bucketSizePtr[bucket];
                        int offset = bucketOffsetPtr[bucket];
                        spdlog::error("bucket: {}, offset: {}, size: {}", bucket, offset, size);
                        for(auto i = 0; i < size; i++){
                            auto entry = i + offset;
                            auto pIndex = bucketPtr[entry];
                            auto point = particlePtr[pIndex].xyz();
                            spdlog::error("\tpoint[{}] => {}", pIndex, point);
                        }
                    }
                });
            });
        });
        });
    }

    void assertNeighbourList(int index, const std::vector<glm::vec3>& neighbourList){
        std::vector<bool> foundNeighbours(neighbourList.size(), false);
        grid.neighbourList.neighbourSizeBuffer.map<int>([&](auto sizePtr){
           grid.neighbourList.neighbourOffsetsBuffer.map<int>([&](auto offsetPtr){
               auto size = sizePtr[index];
               ASSERT_EQ(neighbourList.size(), size) << "neighbour list size did not match" ;
                grid.neighbourList.neighbourListBuffer.map<int>([&](auto listPtr){
                    particleBuffer.map<glm::vec4>([&](auto particlePtr){
                        int offset = offsetPtr[index];
                        int prevIndex = -1;
                        for(int i = 0; i < size; i++){
                            int particleIndex = listPtr[i + offset];
                            glm::vec3 actual = particlePtr[particleIndex].xyz();

                            ASSERT_NE(prevIndex, particleIndex) << fmt::format("duplicate particle found {} at iteration: {}", particleIndex, i);
                            auto found = std::find_if(begin(neighbourList), end(neighbourList), [&](auto expected){ return similar(expected, actual); });

                            foundNeighbours[i] = found != end(neighbourList);
                            prevIndex = particleIndex;
                        }
                        auto foundAllNeighbours = std::all_of(begin(foundNeighbours), end(foundNeighbours), [](auto res){ return res; });
                        ASSERT_TRUE(foundAllNeighbours) << fmt::format("neighbours missing {}", foundAllNeighbours);
                    });
                });
           });
        });
    }

    template<typename Rng, size_t Amount>
    std::vector<glm::vec3> generatePointsAround(const Rng& rng, glm::vec3 origin, float maxDist){
        std::vector<glm::vec3> points;
        for(int i = 0; i < Amount; i++) {
            glm::vec2 uv { rng(), rng()};
            auto point = glm::vec3(origin.xy() + maxDist * sampling::uniformSampleDisk(uv), 0);
            points.push_back(point);
            addParticleAt(point);
        }
        return points;
    }

    template<size_t Amount>
    std::vector<glm::vec3> generateRandomPointsInGrid(float lower, float upper){
        std::vector<glm::vec3> points;
        auto rng = rngFunc(lower, upper, 1 << 20);
        for(int i = 0; i < Amount; i++){
            glm::vec3 point{ rng(), rng(), 0};
            points.push_back(point);
            addParticleAt(point);
        }
        return points;
    }

    std::map<int, std::vector<glm::vec3>> getExpectedNeighbourList(){
        std::map<int, std::vector<glm::vec3>> list;
        float radius = gridSpacing * 0.5;
        float radiusSqr = radius * radius;
        for(auto i = 0; i < particles.size(); i++){
            auto origin = particles[i].xyz();
            std::vector<glm::vec3> neighbours;
            for(auto& point : particles){
                glm::vec3 d = point.xyz() - origin;
                if(dot(d, d) <= radiusSqr){
                    neighbours.push_back(point.xyz());
                }
            }
            list[i] = neighbours;
        }
        return list;
    }
};

TEST_F(PointHashGridBuilderTest, OnePointPerGrid2d){
    gridSpacing = 0.1;
    resolution = glm::vec3{10, 10, 1};
    auto generateParticle = [&](int x, int y, int _) -> std::vector<glm::vec4> {
        glm::vec3 pos = (glm::vec3(x, y, 0) + glm::vec3(0.5f, 0.5f, 0)) * gridSpacing;
        return { {pos, 1} };
    };
    grid.constants.resolution = resolution;
    grid.constants.gridSpacing = gridSpacing;

    createParticles(generateParticle, defaultHash);
    buildHashGrid();
    AssertGrid();
}

TEST_F(PointHashGridBuilderTest, OnePointPerGrid3d){
    gridSpacing = 0.1;
    resolution = glm::vec3{10, 10, 10};
    auto generateParticle = [&](int x, int y, int z) -> std::vector<glm::vec4> {

        glm::vec4 particle;
        glm::vec3 pos = (glm::vec3(x, y, z) + 0.5f) * gridSpacing;
        particle = glm::vec4(pos, 0); // range [0, 1]
        return { particle };
    };
    grid.constants.resolution = resolution;
    grid.constants.gridSpacing = gridSpacing;

    createParticles(generateParticle, defaultHash);
    buildHashGrid();
    AssertGrid();
}

TEST_F(PointHashGridBuilderTest, generateGridWithPointsInNegativeAndPositiveSpace){
    resolution =  glm::vec3 {4, 4, 4};
    gridSpacing = 0.25;
    auto generateParticle = [&](int x, int y, int z) -> std::vector<glm::vec4> {
        glm::vec4 particle;
        glm::vec3 pos = (glm::vec3(x, y, z) + 0.5f) * gridSpacing - 1.0f;
        particle = glm::vec4(pos, 1); // [-1, 1]
        return { particle };
    };
    grid.constants.resolution = resolution;
    grid.constants.gridSpacing = gridSpacing;

    createParticles(generateParticle, defaultHash);
    buildHashGrid();
    AssertGrid();
}

TEST_F(PointHashGridBuilderTest, PointRandomlyScatteredInSpace){
    auto rng = rngFunc<float>(0, 0.9, 1 << 20);
    resolution =  glm::vec3 {4, 4, 4};
    gridSpacing = 0.25;

    auto generateParticle = [&](int x, int y, int z) -> std::vector<glm::vec4> {

        glm::vec4 particle;
        particle.x = rng();
        particle.y = rng();
        particle.z = rng();
        return { particle };
    };
    grid.constants.resolution = resolution;
    grid.constants.gridSpacing = gridSpacing;

    createParticles(generateParticle, defaultHash);
    buildHashGrid();
    AssertGrid();
}

TEST_F(PointHashGridBuilderTest, getNearByKeys2dOriginInTopLeftCorner){
    resolution = glm::vec3{4};
    gridSpacing = 0.25;

    glm::vec3 position = glm::vec3(1, 2, 0) * gridSpacing;
    position.x += gridSpacing * 0.2f;
    position.y += gridSpacing * 0.8f;
    addParticleAt(position);
    grid.constants.resolution = resolution;
    grid.constants.gridSpacing = gridSpacing;

    createParticles();
    buildHashGrid();
    getNearByKeys();
    ASSERT_PRED4(containsNearbyKey, 8, 9, 12, 13) << "Near by keys did not match 8, 9, 12 & 13";
}

TEST_F(PointHashGridBuilderTest, getNearByKeys2dOriginInBottomLeftCorner){
    resolution = glm::vec3{4};
    gridSpacing = 0.25;

    glm::vec3 position = glm::vec3(1, 2, 0) * gridSpacing;
    position.x += gridSpacing * 0.2f;
    position.y += gridSpacing * 0.2f;
    addParticleAt(position);
    grid.constants.resolution = resolution;
    grid.constants.gridSpacing = gridSpacing;

    createParticles();
    buildHashGrid();
    getNearByKeys();
    ASSERT_PRED4(containsNearbyKey, 4, 5, 8, 9) << "Near by keys did not match 4, 5, 8 & 9";
}
TEST_F(PointHashGridBuilderTest, getNearByKeys2dOriginInBottomRightCorner){
    resolution = glm::vec3{4, 4, 1};
    gridSpacing = 0.25;

    glm::vec3 position = glm::vec3(1, 2, 0) * gridSpacing;
    position.x += gridSpacing * 0.8f;
    position.y += gridSpacing * 0.2f;
    addParticleAt(position);
    grid.constants.resolution = resolution;
    grid.constants.gridSpacing = gridSpacing;

    createParticles();
    buildHashGrid();
    getNearByKeys();
    ASSERT_PRED4(containsNearbyKey, 5, 6, 9, 10) << "Near by keys did not match 5, 6, 9, 10";
}

TEST_F(PointHashGridBuilderTest, getNearByKeys2dOriginInTopRightCorner){
    resolution = glm::vec3{4, 4, 4};
    gridSpacing = 0.25;

    glm::vec3 position = glm::vec3(1, 2, 0) * gridSpacing;
    position.x += gridSpacing * 0.8f;
    position.y += gridSpacing * 0.8f;
    addParticleAt(position);

    createParticles();
    buildHashGrid();
    getNearByKeys();
    ASSERT_PRED4(containsNearbyKey, 9, 10, 13, 14) << "Near by keys did not match 9, 10, 13, 14";
}

TEST_F(PointHashGridBuilderTest, getNearByKeys2dOfPointInBottomCornerofCellZero){
    resolution = glm::vec3{4, 4, 1};
    gridSpacing = 0.25;

    addParticleAt(glm::vec3{0});

    createParticles();
    buildHashGrid();
    getNearByKeys();
    ASSERT_PRED4(containsNearbyKey, 0, 3, 12, 15) << "Near by keys did not match 0, 3, 12, 15";
}

TEST_F(PointHashGridBuilderTest, generateNeighbourList){
    resolution = glm::vec3{4, 4, 1};
    gridSpacing = 0.25;

    glm::vec3 origin = glm::vec3(0, 0, 0) * gridSpacing;
    origin.x += gridSpacing * 0.8f;
    origin.y += gridSpacing * 0.2f;
    addParticleAt(origin);

    std::vector<glm::vec3> expectedNeighbourList;

    auto rng = canonicalRng(1 << 20);
    expectedNeighbourList = generatePointsAround< decltype(rng), 10>(rng, origin, gridSpacing * 0.5f);
    expectedNeighbourList.push_back(origin);

    for(int i = 0; i < 5; i++){
        float theta = float(i)/5.0f * glm::two_pi<float>();
        auto position = glm::vec3(origin.xy() +  gridSpacing * glm::vec2(std::cos(theta), std::sin(theta)), 0);
        addParticleAt(position);
    }


    createParticles();
    generateNeighbourList();
    assertNeighbourList(0, expectedNeighbourList);
}

TEST_F(PointHashGridBuilderTest, generateNeighbourListOfRandomPoints){
    resolution = glm::vec3{4, 4, 1};
    gridSpacing = 0.25;

    generateRandomPointsInGrid<100>(0.0f, 1.0f);
    createParticles();

    generateNeighbourList();

    auto expected = getExpectedNeighbourList();
    for(auto& [index, expectedNeighbourList] : expected){
        assertNeighbourList(index, expectedNeighbourList);
    }

}