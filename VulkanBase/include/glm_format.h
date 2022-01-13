#pragma once

#include <string>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

template<glm::length_t L, typename T, glm::qualifier Q>
struct fmt::formatter<glm::vec<L, T, Q>> {
        // Presentation format: 'f' - fixed, 'e' - exponential.
        char presentation = 'f';    // TODO extract and send to fmt;

        // Parses format specifications of the form ['f' | 'e'].
        constexpr auto parse(format_parse_context& ctx) {
            auto it = ctx.begin(), end = ctx.end();
            if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;

            if (it != end && *it != '}')
                throw format_error("invalid format");

            return it;
        }


        template <typename FormatContext>
        auto format(const glm::vec<L, T, Q>& v, FormatContext& ctx) {

            if constexpr (L == 2) {
                return format_to(
                        ctx.out(),
                        presentation == 'f' ? "[{:.3f}, {:.3f}]" : "[{:.3e}, {:.3e}]",
                        v.x, v.y);
            }else if constexpr (L == 3){
                return format_to(
                        ctx.out(),
                        presentation == 'f' ? "[{:.3f}, {:.3f}, {:.3f}]" : "[{:.1e}, {:.1e}, {:.1e}]",
                        v.x, v.y, v.z);
            } else if constexpr (L == 4){
                return format_to(
                        ctx.out(),
                        presentation == 'f' ? "[{:.3f}, {:.3f}, {:.3f}, {:.3f}]" : "[{:.1e}, {:.1e}, {:.1e}, {:.1e}]",
                        v.x, v.y, v.z, v.w);
            }else {
                std::string msg = fmt::format("Invalid vector length: {}", L);
                throw std::runtime_error{msg};
            }
        }
};


template<typename T, glm::qualifier Q>
struct fmt::formatter<glm::qua<T, Q>> {
    // Presentation format: 'f' - fixed, 'e' - exponential.
    char presentation = 'f';

    constexpr auto parse(format_parse_context& ctx) {

        auto it = ctx.begin(), end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }


    template <typename FormatContext>
    auto format(const glm::qua<T, Q>& q, FormatContext& ctx) {
        return format_to(
                ctx.out(),
                presentation == 'f' ? "[{:.3f}, < {:.3f}, {:.3f}, {:.3f}>]" : "[{:.1e}, <{:.1e}, {:.1e}, {:.1e} >]",
                q.w, q.x, q.y, q.z);
    }
};


template<glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
struct fmt::formatter<glm::mat<C, R, T, Q>> {
    // Presentation format: 'f' - fixed, 'e' - exponential.
    char presentation = 'f';

    constexpr auto parse(format_parse_context& ctx) {

        auto it = ctx.begin(), end = ctx.end();
        if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;

        // Check if reached the end of the range:
        if (it != end && *it != '}')
            throw format_error("invalid format");

        // Return an iterator past the end of the parsed range:
        return it;
    }


    template <typename FormatContext>
    auto format(const glm::mat<C, R, T, Q>& mat, FormatContext& ctx) {

        if constexpr (R == 2) {
            return format_to(
                    ctx.out(),
                    presentation == 'f' ? "{:.3f}\n{:.3f}" : "{:.3e}\n{:.3e}]",
                    glm::row(mat, 0), glm::row(mat, 1));
        }else if constexpr (R == 3){
            return format_to(
                    ctx.out(),
                    presentation == 'f' ? "{:.3f}\n{:.3f}\n{:.3f}" : "{:.1e}\n{:.1e}\n{:.1e}",
                    glm::row(mat , 0), glm::row(mat, 1), glm::row(mat, 2));
        } else if constexpr (R == 4){
            return format_to(
                    ctx.out(),
                    presentation == 'f' ? "{:f}\n{:f}\n{:f}\n{:f}" : "{:e}\n{:e}\n{:e}\n{:e}",
                    glm::row(mat , 0), glm::row(mat, 1), glm::row(mat, 2), glm::row(mat, 3));
        }else {
            std::string msg = fmt::format("No format defined for mat{}{}", C, R);
            throw std::runtime_error{msg};
        }
    }
};