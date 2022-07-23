#pragma once

#define IX(i, j) ((i)+(N+2)*(j))

void add_source ( int N, float * x, const float * s, float dt );
void add_source0 ( int N, float * x, const float * s, float dt );

void diffuse ( int N, int b, float * x, const float * x0, float diff, float dt );
void diffuse0 ( int N, int b, float * x, const float * x0, float diff, float dt, int iterations = 20 );

void advect ( int N, int b, float * d, const float * d0, const float * u, const float * v, float dt );

void dens_step ( int N, float * x, float * x0, float * u, float * v, float diff, float dt );

void vel_step ( int N, float * u, float * v, float * u0, float * v0, float visc, float dt );
void vel_step0 ( int N, float * u, float * v, float * u0, float * v0, float visc, float dt, int iterations = 20);

void divergence(int N, const float *u, const float *v, float *div);

void pressureSolver(int N, const float* div, float* p, int iter = 20);

void pressureSolverJacobi(int N, const float* div, float* p, int iter = 20);

void project ( int N, float * u, float * v, float * p, float * div );
void project0 ( int N, float * u, float * v, float * p, float * div, int iterations = 20);

void set_bnd ( int N, int b, float * x );

void fluid_sim(int N, float dt, float diff, float visc, float *u, float* v, float* u_prev, float* v_prev, float* dens, float* dens_prev);