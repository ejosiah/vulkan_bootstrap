#pragma once

inline glm::mat3 minor(const glm::mat4& m, int row, int column){
    glm::mat3 minor{};

    int yy =  0;
    for(int y = 0; y < 4; y++){
        if(y == column){
            continue;
        }

        int xx = 0;
        for(int x = 0; x < 4; x++){
            if(x == row){
                continue;
            }
            minor[yy][xx] = m[y][x];
            xx++;
        }
        yy++;
    }
    return minor;
}

inline float cofactor(const glm::mat4& m, int row, int column){
    const auto m3 = minor(m, row, column);
    const auto C = float( glm::pow(-1, row + 1 + column + 1)) * glm::determinant(m3);
    return C;
}