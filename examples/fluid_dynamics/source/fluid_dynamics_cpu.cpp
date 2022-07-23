#include "fluid_dynamics_cpu.h"
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>

//#define IX(i, j) ((i)*(N+2)+(j))

#define SWAP(x0, x) {float *tmp=x0;x0=x;x=tmp;}

void add_source(int N, float *x, const float *s, float dt) {
    int  size = (N + 2) * (N + 2);
    for (int i = 0; i < size; i++) x[i] += dt * s[i];
}
void add_source0(int N, float *x, const float *s, float dt) {
    int  size = (N + 2) * (N + 2);
    for (int i = 0; i < size; i++) x[i] = s[i];
}

void diffuse(int N, int b, float *x, const float *x0, float diff, float dt) {
    float a = dt * diff * N * N;
    for (int k = 0; k < 20; k++) {
        for (int i = 1; i <= N; i++) {
            for (int j = 1; j <= N; j++) {
                x[IX(i, j)] = (x0[IX(i, j)] + a * (x[IX(i - 1, j)] + x[IX(i + 1, j)] +
                                                   x[IX(i, j - 1)] + x[IX(i, j + 1)])) / (1 + 4 * a);
            }
        }
        set_bnd(N, b, x);
    }
}

void diffuse0(int N, int b, float *x, const float *x0, float diff, float dt, int iterations) {
    float a = dt * diff * N * N;

    int size = (N+2)*(N+2);
    std::vector<float> oldX(size);
    for (int k = 0; k < iterations; k++) {
        std::memcpy(oldX.data(), x, sizeof(float) * size);
        for (int i = 1; i <= N; i++) {
            for (int j = 1; j <= N; j++) {
                x[IX(i, j)] = (x0[IX(i, j)] + a * (oldX[IX(i - 1, j)] + oldX[IX(i + 1, j)] +
                                                   oldX[IX(i, j - 1)] + oldX[IX(i, j + 1)])) / (1 + 4 * a);
            }
        }
        set_bnd(N, b, x);
    }
}

void advect(int N, int b, float *d, const float *d0, const float *u, const float *v, float dt) {
    float dt0 = dt * N;
    for (int i = 1; i <= N; i++) {
        for (int j = 1; j <= N; j++) {
            float _u = u[IX(i, j)];
            float _v = v[IX(i, j)];
            float x = i - dt0 * _u;
            float y = j - dt0 * _v;
            x = glm::isnan(x) ? 0 : x;
            y = glm::isnan(y) ? 0 : y;

            if (x < 0.5) x = 0.5;
            if (x > N + 0.5) x = N + 0.5;
            int i0 = (int) x;
            int i1 = i0 + 1;
            if (y < 0.5) y = 0.5;
            if (y > N + 0.5) y = N + 0.5;
            int j0 = (int) y;
            int j1 = j0 + 1;
            float s1 = x - i0;
            float s0 = 1 - s1;
            float t1 = y - j0;
            float t0 = 1 - t1;
            float q0 = d0[IX(i0, j0)];
            float q1 = d0[IX(i0, j1)];
            float q2 = d0[IX(i1, j0)];
            float q3 = d0[IX(i1, j1)];

            d[IX(i, j)] = s0 * (t0 * q0 + t1 * q1)
                          + s1 * (t0 * q2 + t1 * q3);
        }
    }
    set_bnd(N, b, d);
}

void dens_step(int N, float *x, float *x0, float *u, float *v, float diff, float dt) {
    add_source(N, x, x0, dt);
    SWAP (x0, x);
    diffuse(N, 0, x, x0, diff, dt);
    SWAP (x0, x);
    advect(N, 0, x, x0, u, v, dt);
}

void vel_step(int N, float *u, float *v, float *u0, float *v0, float visc, float dt) {
    add_source(N, u, u0, dt);
    add_source(N, v, v0, dt);
    SWAP (u0, u);
    diffuse(N, 1, u, u0, visc, dt);
    SWAP (v0, v);
    diffuse(N, 2, v, v0, visc, dt);
    project(N, u, v, u0, v0);
    SWAP (u0, u);
    SWAP (v0, v);
    advect(N, 1, u, u0, u0, v0, dt);
    advect(N, 2, v, v0, u0, v0, dt);
    project(N, u, v, u0, v0);
}

void vel_step0(int N, float *u, float *v, float *u0, float *v0, float visc, float dt, int iterations) {

    fmt::print("in: {}, out: {}\n", (uint64_t)u0, (uint64_t)u);
    diffuse0(N, 1, u, u0, visc, dt, iterations);
    diffuse0(N, 2, v, v0, visc, dt, iterations);
    project0(N, u, v, u0, v0, iterations);

    SWAP (u0, u);
    SWAP (v0, v);
    fmt::print("in: {}, out: {}\n", (uint64_t)u0, (uint64_t)u);
    advect(N, 1, u, u0, u0, v0, dt);
    advect(N, 2, v, v0, u0, v0, dt);
    project0(N, u, v, u0, v0, iterations);
}

void project(int N, float *u, float *v, float *p, float *div) {
    float h = 1.0f / N;
    for (int i = 1; i <= N; i++) {
        for (int j = 1; j <= N; j++) {
            div[IX(i, j)] = -0.5f * h * (u[IX(i + 1, j)] - u[IX(i - 1, j)] +
                                         v[IX(i, j + 1)] - v[IX(i, j - 1)]);
            p[IX(i, j)] = 0;
        }
    }
    set_bnd(N, 0, div);
    set_bnd(N, 0, p);
    for (int k = 0; k < 1024; k++) {
        for (int i = 1; i <= N; i++) {
            for (int j = 1; j <= N; j++) {
                p[IX(i, j)] = (div[IX(i, j)] + p[IX(i - 1, j)] + p[IX(i + 1, j)] +
                               p[IX(i, j - 1)] + p[IX(i, j + 1)]) / 4;
            }
        }
        set_bnd(N, 0, p);
    }
    for (int i = 1; i <= N; i++) {
        for (int j = 1; j <= N; j++) {
            u[IX(i, j)] -= 0.5f * (p[IX(i + 1, j)] - p[IX(i - 1, j)]) / h;
            v[IX(i, j)] -= 0.5f * (p[IX(i, j + 1)] - p[IX(i, j - 1)]) / h;
        }
    }
    set_bnd(N, 1, u);
    set_bnd(N, 2, v);
}

void project0(int N, float *u, float *v, float *p, float *div, int iterations) {
    float h = 1.0f / N;
    for (int i = 1; i <= N; i++) {
        for (int j = 1; j <= N; j++) {
            div[IX(i, j)] = -0.5f * h * (u[IX(i + 1, j)] - u[IX(i - 1, j)] +
                                         v[IX(i, j + 1)] - v[IX(i, j - 1)]);
            p[IX(i, j)] = 0;
        }
    }
    set_bnd(N, 0, div);
    set_bnd(N, 0, p);

    std::vector<float> p0((N+2) * (N+2));
    for (int k = 0; k < iterations; k++) {
        std::memcpy(p0.data(), p, sizeof(float) * (N+2) * (N+2));
        for (int i = 1; i <= N; i++) {
            for (int j = 1; j <= N; j++) {
                p[IX(i, j)] = (div[IX(i, j)] + p0[IX(i - 1, j)] + p0[IX(i + 1, j)] +
                               p0[IX(i, j - 1)] + p0[IX(i, j + 1)]) / 4;
            }
        }
        set_bnd(N, 0, p);
    }
    for (int i = 1; i <= N; i++) {
        for (int j = 1; j <= N; j++) {
            u[IX(i, j)] -= 0.5f * (p[IX(i + 1, j)] - p[IX(i - 1, j)]) / h;
            v[IX(i, j)] -= 0.5f * (p[IX(i, j + 1)] - p[IX(i, j - 1)]) / h;
        }
    }
    set_bnd(N, 1, u);
    set_bnd(N, 2, v);
}

void divergence(int N, const float *u, const float *v, float *div){
    float h = 1.0f / N;
    for (int i = 1; i <= N; i++) {
        for (int j = 1; j <= N; j++) {
            div[IX(i, j)] = -0.5f * h * (u[IX(i + 1, j)] - u[IX(i - 1, j)] +
                                         v[IX(i, j + 1)] - v[IX(i, j - 1)]);
        }
    }

    set_bnd(N, 0, div);
}

void pressureSolver(int N, const float* div, float* p, int iter){
    for (int k = 0; k < iter; k++) {
        for (int i = 1; i <= N; i++) {
            for (int j = 1; j <= N; j++) {
                p[IX(i, j)] = (div[IX(i, j)] + p[IX(i - 1, j)] + p[IX(i + 1, j)] +
                               p[IX(i, j - 1)] + p[IX(i, j + 1)]) / 4;
            }
        }
        set_bnd(N, 0, p);
    }
}

void pressureSolverJacobi(int N, const float* div, float* p, int iter){
    std::vector<float> p0((N+2) * (N+2));

    for (int k = 0; k < iter; k++) {
        std::memcpy(p0.data(), p, sizeof(float) * (N+2) * (N+2));
        for (int i = 1; i <= N; i++) {
            for (int j = 1; j <= N; j++) {
                auto a = p0[IX(i - 1, j)];
                auto b = p0[IX(i + 1, j)];
                auto c = p0[IX(i, j - 1)];
                auto d = p0[IX(i, j + 1)];
                p[IX(i, j)] = (div[IX(i, j)] + a + b + c + d) / 4;
            }
        }
        set_bnd(N, 0, p);
    }
}

void set_bnd(int N, int b, float *x) {
    int i;
    for (i = 1; i <= N; i++) {
        x[IX(0, i)] = b == 1 ? -x[IX(1, i)] : x[IX(1, i)];
        x[IX(N + 1, i)] = b == 1 ? -x[IX(N, i)] : x[IX(N, i)];
        x[IX(i, 0)] = b == 2 ? -x[IX(i, 1)] : x[IX(i, 1)];
        x[IX(i, N + 1)] = b == 2 ? -x[IX(i, N)] : x[IX(i, N)];
    }
    x[IX(0, 0)] = 0.5f * (x[IX(1, 0)] + x[IX(0, 1)]);
    x[IX(0, N + 1)] = 0.5f * (x[IX(1, N + 1)] + x[IX(0, N)]);
    x[IX(N + 1, 0)] = 0.5f * (x[IX(N, 0)] + x[IX(N + 1, 1)]);
    x[IX(N + 1, N + 1)] = 0.5f * (x[IX(N, N + 1)] + x[IX(N + 1, N)]);
}

void fluid_sim(int N, float dt, float diff, float visc, float *u, float *v, float *u_prev, float *v_prev, float *dens,
               float *dens_prev) {

    vel_step(N, u, v, u_prev, v_prev, visc, dt);

    dens_step(N, dens, dens_prev, u, v, diff, dt);
}
