#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "algo.h"

void
init_algorithm(struct curve *curve, double T, double sigma)
{
    curve->count = 0;
    curve->next_index = 0;
    
    curve->n = 0;
    curve->c = 0;
    curve->c_bar = 0;
    curve->rho = 0;
    curve->T = T;
    curve->sigma = sigma;
}

void
add_data(struct curve *curve, double new_data)
{
    if (curve->count < BUFFER_SIZE) {
        curve->data[curve->next_index] = new_data;
        curve->count++;
    } else {
        curve->data[curve->next_index] = new_data;

        if (curve->c == curve->next_index) {
            curve->c = (curve->next_index + 1) % BUFFER_SIZE;
        }
        if (curve->c_bar == curve->next_index) {
            curve->c_bar = (curve->next_index + 1) % BUFFER_SIZE;
        }
    }

    
    curve->next_index = (curve->next_index + 1) % BUFFER_SIZE;
}

void
process_data(struct curve *curve)
{
    if (curve->count < 2) {
        printf("Недостаточно данных для обработки\n");
    }

    int start_index = (curve->next_index - curve->count + BUFFER_SIZE) % BUFFER_SIZE;

    while (curve->n < curve->count - 1) {
        curve->n++;
        int current_n = (start_index + curve->n) % BUFFER_SIZE;
        int current_c = (start_index + curve->c) % BUFFER_SIZE;
        int current_c_bar = (start_index + curve->c_bar) % BUFFER_SIZE;

        
        double xn = curve->data[current_n];
        double xc = curve->data[current_c];

        double xc_bar = curve->data[current_c_bar];

        bool lower_ok = curve->n >= curve->rho * (xn - xc - curve->T) + curve->c;
        bool upper_ok = curve->n <= curve->rho * (xn - xc_bar) + curve->sigma + curve->c_bar;

        if (!lower_ok) {
            printf("WARNING: Нижнее ограничение нарушено в точке %d\n", curve->n);
            curve->rho = (curve->n - curve->c) / (xn - xc);
            curve->c = curve->c_bar = curve->n;
        } else if (!upper_ok) {
            printf("WARNING: Верхнее ограничение нарушено в точке %d\n", curve->n);
            curve->rho = (curve->n - curve->c_bar) / (xn - xc_bar);
            curve->c = curve->c_bar = curve->n;
        }
    }
}
