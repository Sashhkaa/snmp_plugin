#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 100
struct curve {
    double data[100];
    int count;
    int next_index;

    int n;
    int c;
    int c_bar;
    double rho;
    double T;
    double sigma;
};

void
init_algorithm(struct curve *curve, double T, double sigma);
void
add_data(struct curve *curve, double new_data);
void
process_data(struct curve *curve);