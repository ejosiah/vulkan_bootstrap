#pragma once
#include <optional>
#include <algorithm>
#include <vector>

namespace ctn {

    template<typename ContainerA, typename ContainerB>
    inline bool containsAll(const ContainerA& container, const ContainerB& args){
        return std::all_of(begin(args), end(args), [&](auto& it0){
           return std::any_of(begin(container), end(container), [&](auto& it1){
               return it0 == it1;
           }) ;
        });
    }

    template<typename Predicate, typename T>
    inline std::optional<T> find_if(const std::vector<T>& container, Predicate&& predicate){
        auto itr = std::find_if(begin(container), end(container), predicate);
        if(itr != end(container)) return *itr;
        return {};
    }

    template<typename Container, typename T>
    inline std::optional<T> find(const Container& container, T t){
        auto itr = std::find(begin(container), end(container), t);
        if(itr != end(container)) return *itr;
        return {};
    }
}
