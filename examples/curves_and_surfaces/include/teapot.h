#pragma once

#include <tuple>
#include <vector>
#include <glm/glm.hpp>
#include <algorithm>
#include <numeric>

const int patchdata[][16] =
        {
                /* rim */
                { 102, 103, 104, 105, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
                /* body */
                { 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27 },
                { 24, 25, 26, 27, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40 },
                /* lid */
                { 96, 96, 96, 96, 97, 98, 99, 100, 101, 101, 101, 101, 0, 1, 2, 3, },
                { 0, 1, 2, 3, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117 },
                /* bottom */
                { 118, 118, 118, 118, 124, 122, 119, 121, 123, 126, 125, 120, 40, 39, 38, 37 },
                /* handle */
                { 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56 },
                { 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 28, 65, 66, 67 },
                /* spout */
                { 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83 },
                { 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95 }
        };

const float cpdata[][3] =
        {
                { 0.2f, 0.f, 2.7f },
                { 0.2f, -0.112f, 2.7f },
                { 0.112f, -0.2f, 2.7f },
                { 0.f, -0.2f, 2.7f },
                { 1.3375f, 0.f, 2.53125f },
                { 1.3375f, -0.749f, 2.53125f },
                { 0.749f, -1.3375f, 2.53125f },
                { 0.f, -1.3375f, 2.53125f },
                { 1.4375f, 0.f, 2.53125f },
                { 1.4375f, -0.805f, 2.53125f },
                { 0.805f, -1.4375f, 2.53125f },
                { 0.f, -1.4375f, 2.53125f },
                { 1.5f, 0.f, 2.4f },
                { 1.5f, -0.84f, 2.4f },
                { 0.84f, -1.5f, 2.4f },
                { 0.f, -1.5f, 2.4f },
                { 1.75f, 0.f, 1.875f },
                { 1.75f, -0.98f, 1.875f },
                { 0.98f, -1.75f, 1.875f },
                { 0.f, -1.75f, 1.875f },
                { 2.f, 0.f, 1.35f },
                { 2.f, -1.12f, 1.35f },
                { 1.12f, -2.f, 1.35f },
                { 0.f, -2.f, 1.35f },
                { 2.f, 0.f, 0.9f },
                { 2.f, -1.12f, 0.9f },
                { 1.12f, -2.f, 0.9f },
                { 0.f, -2.f, 0.9f },
                { -2.f, 0.f, 0.9f },
                { 2.f, 0.f, 0.45f },
                { 2.f, -1.12f, 0.45f },
                { 1.12f, -2.f, 0.45f },
                { 0.f, -2.f, 0.45f },
                { 1.5f, 0.f, 0.225f },
                { 1.5f, -0.84f, 0.225f },
                { 0.84f, -1.5f, 0.225f },
                { 0.f, -1.5f, 0.225f },
                { 1.5f, 0.f, 0.15f },
                { 1.5f, -0.84f, 0.15f },
                { 0.84f, -1.5f, 0.15f },
                { 0.f, -1.5f, 0.15f },
                { -1.6f, 0.f, 2.025f },
                { -1.6f, -0.3f, 2.025f },
                { -1.5f, -0.3f, 2.25f },
                { -1.5f, 0.f, 2.25f },
                { -2.3f, 0.f, 2.025f },
                { -2.3f, -0.3f, 2.025f },
                { -2.5f, -0.3f, 2.25f },
                { -2.5f, 0.f, 2.25f },
                { -2.7f, 0.f, 2.025f },
                { -2.7f, -0.3f, 2.025f },
                { -3.f, -0.3f, 2.25f },
                { -3.f, 0.f, 2.25f },
                { -2.7f, 0.f, 1.8f },
                { -2.7f, -0.3f, 1.8f },
                { -3.f, -0.3f, 1.8f },
                { -3.f, 0.f, 1.8f },
                { -2.7f, 0.f, 1.575f },
                { -2.7f, -0.3f, 1.575f },
                { -3.f, -0.3f, 1.35f },
                { -3.f, 0.f, 1.35f },
                { -2.5f, 0.f, 1.125f },
                { -2.5f, -0.3f, 1.125f },
                { -2.65f, -0.3f, 0.9375f },
                { -2.65f, 0.f, 0.9375f },
                { -2.f, -0.3f, 0.9f },
                { -1.9f, -0.3f, 0.6f },
                { -1.9f, 0.f, 0.6f },
                { 1.7f, 0.f, 1.425f },
                { 1.7f, -0.66f, 1.425f },
                { 1.7f, -0.66f, 0.6f },
                { 1.7f, 0.f, 0.6f },
                { 2.6f, 0.f, 1.425f },
                { 2.6f, -0.66f, 1.425f },
                { 3.1f, -0.66f, 0.825f },
                { 3.1f, 0.f, 0.825f },
                { 2.3f, 0.f, 2.1f },
                { 2.3f, -0.25f, 2.1f },
                { 2.4f, -0.25f, 2.025f },
                { 2.4f, 0.f, 2.025f },
                { 2.7f, 0.f, 2.4f },
                { 2.7f, -0.25f, 2.4f },
                { 3.3f, -0.25f, 2.4f },
                { 3.3f, 0.f, 2.4f },
                { 2.8f, 0.f, 2.475f },
                { 2.8f, -0.25f, 2.475f },
                { 3.525f, -0.25f, 2.49375f },
                { 3.525f, 0.f, 2.49375f },
                { 2.9f, 0.f, 2.475f },
                { 2.9f, -0.15f, 2.475f },
                { 3.45f, -0.15f, 2.5125f },
                { 3.45f, 0.f, 2.5125f },
                { 2.8f, 0.f, 2.4f },
                { 2.8f, -0.15f, 2.4f },
                { 3.2f, -0.15f, 2.4f },
                { 3.2f, 0.f, 2.4f },
                { 0.f, 0.f, 3.15f },
                { 0.8f, 0.f, 3.15f },
                { 0.8f, -0.45f, 3.15f },
                { 0.45f, -0.8f, 3.15f },
                { 0.f, -0.8f, 3.15f },
                { 0.f, 0.f, 2.85f },
                { 1.4f, 0.f, 2.4f },
                { 1.4f, -0.784f, 2.4f },
                { 0.784f, -1.4f, 2.4f },
                { 0.f, -1.4f, 2.4f },
                { 0.4f, 0.f, 2.55f },
                { 0.4f, -0.224f, 2.55f },
                { 0.224f, -0.4f, 2.55f },
                { 0.f, -0.4f, 2.55f },
                { 1.3f, 0.f, 2.55f },
                { 1.3f, -0.728f, 2.55f },
                { 0.728f, -1.3f, 2.55f },
                { 0.f, -1.3f, 2.55f },
                { 1.3f, 0.f, 2.4f },
                { 1.3f, -0.728f, 2.4f },
                { 0.728f, -1.3f, 2.4f },
                { 0.f, -1.3f, 2.4f },
                { 0.f, 0.f, 0.f },
                { 1.425f, -0.798f, 0.f },
                { 1.5f, 0.f, 0.075f },
                { 1.425f, 0.f, 0.f },
                { 0.798f, -1.425f, 0.f },
                { 0.f, -1.5f, 0.075f },
                { 0.f, -1.425f, 0.f },
                { 1.5f, -0.84f, 0.075f },
                { 0.84f, -1.5f, 0.075f }
        };

void getPatch(int patchNum, glm::vec3 patch[][4], bool reverseV);
void buildPatchReflect(int patchNum, float *v, int &index, bool reflectX, bool reflectY);
void generatePatches(float * v);
void buildPatch(glm::vec3 patch[][4], float *v, int &index, glm::mat3 reflect);

std::tuple<std::vector<glm::vec3>, std::vector<uint16_t>> teapotPatch(){
    std::vector<glm::vec3> points(32 * 16);
    std::vector<uint16_t> indices(32 * 16); // merge vertex copies
    generatePatches(reinterpret_cast<float*>(points.data()));
    std::iota(begin(indices), end(indices), 0);
    return std::make_tuple(points, indices);
}

void generatePatches(float * v) {
    int idx = 0;

    // Build each patch
    // The rim
    buildPatchReflect(0, v, idx, true, true);
    // The body
    buildPatchReflect(1, v, idx, true, true);
    buildPatchReflect(2, v, idx, true, true);
    // The lid
    buildPatchReflect(3, v, idx, true, true);
    buildPatchReflect(4, v, idx, true, true);
    // The bottom
    buildPatchReflect(5, v, idx, true, true);
    // The handle
    buildPatchReflect(6, v, idx, false, true);
    buildPatchReflect(7, v, idx, false, true);
    // The spout
    buildPatchReflect(8, v, idx, false, true);
    buildPatchReflect(9, v, idx, false, true);
}

void buildPatchReflect(int patchNum,
                       float *v, int &index, bool reflectX, bool reflectY)
{
    glm::vec3 patch[4][4];
    glm::vec3 patchRevV[4][4];
    getPatch(patchNum, patch, false);
    getPatch(patchNum, patchRevV, true);

    // Patch without modification
    buildPatch(patchRevV, v, index, glm::mat3(1.0f));

    // Patch reflected in x
    if (reflectX) {
        buildPatch(patch, v,
                   index, glm::mat3(glm::vec3(-1.0f, 0.0f, 0.0f),
                                    glm::vec3(0.0f, 1.0f, 0.0f),
                                    glm::vec3(0.0f, 0.0f, 1.0f)));
    }

    // Patch reflected in y
    if (reflectY) {
        buildPatch(patch, v,
                   index, glm::mat3(glm::vec3(1.0f, 0.0f, 0.0f),
                                    glm::vec3(0.0f, -1.0f, 0.0f),
                                    glm::vec3(0.0f, 0.0f, 1.0f)));
    }

    // Patch reflected in x and y
    if (reflectX && reflectY) {
        buildPatch(patchRevV, v,
                   index, glm::mat3(glm::vec3(-1.0f, 0.0f, 0.0f),
                                    glm::vec3(0.0f, -1.0f, 0.0f),
                                    glm::vec3(0.0f, 0.0f, 1.0f)));
    }
}

void buildPatch(glm::vec3 patch[][4],
                float *v, int &index, glm::mat3 reflect)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            glm::vec3 pt = reflect * patch[i][j];

            v[index] = pt.x;
            v[index + 1] = pt.y;
            v[index + 2] = pt.z;

            index += 3;
        }
    }
}

void getPatch(int patchNum, glm::vec3 patch[][4], bool reverseV)
{
    for (int u = 0; u < 4; u++) {          // Loop in u direction
        for (int v = 0; v < 4; v++) {     // Loop in v direction
            if (reverseV) {
                patch[u][v] = glm::vec3(
                        cpdata[patchdata[patchNum][u * 4 + (3 - v)]][0],
                        cpdata[patchdata[patchNum][u * 4 + (3 - v)]][1],
                        cpdata[patchdata[patchNum][u * 4 + (3 - v)]][2]
                );
            }
            else {
                patch[u][v] = glm::vec3(
                        cpdata[patchdata[patchNum][u * 4 + v]][0],
                        cpdata[patchdata[patchNum][u * 4 + v]][1],
                        cpdata[patchdata[patchNum][u * 4 + v]][2]
                );
            }
        }
    }
}
