#include "VulkanFixture.hpp"
#include "PointHashGrid.hpp"

inline bool similar(const Particle& a, const Particle& b, float epsilon = 1E-7){
    return closeEnough(a.position.x, b.position.x, epsilon)
           && closeEnough(a.position.y, b.position.y, epsilon)
           && closeEnough(a.position.z, b.position.z, epsilon);
};

class PointHashGridBuilderTest : public VulkanFixture{
protected:
    VulkanBuffer particleBuffer;
    PointHashGrid grid;
    glm::vec3 resolution{1};
    float gridSpacing = 1;

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
        particles.clear();
        expectedHashGrid.clear();
    }

    void createParticleBuffer(const std::vector<Particle> particles){
        particleBuffer = device.createCpuVisibleBuffer(particles.data(), sizeof(Particle) * particles.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }


    template<typename Generate, typename Hash>
    void createParticles(Generate&& generate, Hash&& hash){
        auto res = grid.constants.resolution;
        auto gridSpacing = grid.constants.gridSpacing;
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
        grid.constants.numParticles = particles.size();
    }

    void createParticles(){
        createParticleBuffer(this->particles);
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
        grid.bucketBuffer.map<int>([&](auto bucketPtr){
            grid.bucketSizeBuffer.map<int>([&](auto bucketSizePtr){
                particleBuffer.map<Particle>([&](auto particlePtr){

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
        grid.scan(commandBuffer);
    }


    void buildHashGrid(){
        grid = PointHashGrid{&device, &descriptorPool, &particleBuffer, resolution, gridSpacing};
        execute([&](auto commandBuffer){
            grid.buildHashGrid(commandBuffer);
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

    std::function<bool(int, int, int, int)> containsNearbyKey = [&](int key0, int key1, int key2, int key3){
        std::array<bool, 4> predicates{};
        grid.nearByKeys.map<int>([&](auto ptr){
            for(int i = 0; i < 8; i++){
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
        Particle particle;
        particle.position = glm::vec4(position, 0);
        particles.push_back(particle);
    }

    void addParticleAt(float x, float y, float z = 0){
        addParticleAt({x, y, z});
    }

    void getNearByKeys(){
        execute([&](auto commandBuffer){
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, grid.pipeline("point_hash_grid_unit_test"));
            vkCmdPushConstants(commandBuffer, grid.layout("point_hash_grid_unit_test"), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                               sizeof(grid.constants), &grid.constants);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, grid.layout("point_hash_grid_unit_test"), 0, 1, &grid.unitTestDescriptorSet, 0, nullptr);
            vkCmdDispatch(commandBuffer, 1, 1, 1);

        });
    }
};

TEST_F(PointHashGridBuilderTest, OnePointPerGrid2d){
    gridSpacing = 0.1;
    resolution = glm::vec3{10, 10, 1};
    auto generateParticle = [&](int x, int y, int _) -> std::vector<Particle> {
        Particle particle;
        glm::vec3 pos = (glm::vec3(x, y, 0) + glm::vec3(0.5f, 0.5f, 0)) * gridSpacing;
        particle.position = glm::vec4(pos, 1);
        return { particle };
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
    auto generateParticle = [&](int x, int y, int z) -> std::vector<Particle> {

        Particle particle;
        glm::vec3 pos = (glm::vec3(x, y, z) + 0.5f) * gridSpacing;
        particle.position = glm::vec4(pos, 0); // range [0, 1]
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
    auto generateParticle = [&](int x, int y, int z) -> std::vector<Particle> {
        Particle particle;
        glm::vec3 pos = (glm::vec3(x, y, z) + 0.5f) * gridSpacing - 1.0f;
        particle.position = glm::vec4(pos, 1); // [-1, 1]
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

    auto generateParticle = [&](int x, int y, int z) -> std::vector<Particle> {

        Particle particle;
        particle.position.x = rng();
        particle.position.y = rng();
        particle.position.z = rng();
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
    resolution = glm::vec3{4, 4, 4};
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