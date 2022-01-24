#pragma once
#include <vector>
#include <cstdint>

inline std::vector<std::vector<int>> pascalsTriangle(int index){
    std::vector<std::vector<int>> result;
    std::vector<int> row{0};
    result.push_back(row);
    if(index == 0){
        return result;
    }
    row = std::vector<int>{1, 1};
    result.push_back(row);
    if(index == 1){
        return result;
    }

    for(int i = 2; i < index; i++){
        auto& prev = result[i - 1];
        std::vector<int> row;
        row.push_back(prev.front());
        for(int j = 0; j < prev.size() - 1; j++){
            auto entry = prev[j] + prev[j+1];
            row.push_back(entry);
        }
        row.push_back(prev.back());
        result.push_back(row);
    }
    return result;
}