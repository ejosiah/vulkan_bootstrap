#pragma once

#include <assimp/types.h>
#include <glm/glm.hpp>

inline glm::vec3 to_vec3(const Assimp::aiVector3D& v){
    return {v.x, v.y, v.z};
}

inline glm::vec4 to_vec4(const Assimp::aiColor4D& c){
    return {c.r, c.g, c.b, c.a};
}

inline glm::mat to_mat4(const aiMatrix4x4& matrix){
    glm::mat4 mat;
    //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
    mat[0][0] = matrix.a1; mat[1][0] = matrix.a2; mat[2][0] = matrix.a3; mat[3][0] = matrix.a4;
    mat[0][1] = matrix.b1; mat[1][1] = matrix.b2; mat[2][1] = matrix.b3; mat[3][1] = matrix.b4;
    mat[0][2] = matrix.c1; mat[1][2] = matrix.c2; mat[2][2] = matrix.c3; mat[3][2] = matrix.c4;
    mat[0][3] = matrix.d1; mat[1][3] = matrix.d2; mat[2][3] = matrix.d3; mat[3][3] = matrix.d4;
    return mat;
}

inline glm::mat3 to_mat3(const aiMatrix3x3& matrix){
    glm::mat3 mat;
    //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
    mat[0][0] = matrix.a1; mat[1][0] = matrix.a2; mat[2][0] = matrix.a3;
    mat[0][1] = matrix.b1; mat[1][1] = matrix.b2; mat[2][1] = matrix.b3;
    mat[0][2] = matrix.c1; mat[1][2] = matrix.c2; mat[2][2] = matrix.c3;
    return mat;
}

inline glm::quat to_quat(const aiQuaternion& q){
    return glm::quat{q.w, q.x, q.y, q.z};
}