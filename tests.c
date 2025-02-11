#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "detect_outgoing_msgs.h"

void
determine_sigma_T(int *arrival_times,
                  int n, double *sigma,
                  double *T)
{

    if (n <= 1) {
        *sigma = 1.0;
        *T = 10.0;
        return;
    }
    
    double max_gap = 0;
    for (int i = 1; i < n; i++) {
        double gap = arrival_times[i] - arrival_times[i - 1];
        if (gap > max_gap) {
            max_gap = gap;
        }
    }
    
    *sigma = (double)n / max_gap;
    *T = max_gap;
}

void test_detect_outgoing_and_update_critical() {
    int arrival_times[] = {0, 10, 20, 30, 50, 70};
    int n = 6;
    double sigma, T;
    determine_sigma_T(arrival_times, n, &sigma, &T);
    double lower_curve = sigma;
    double upper_curve = T;
    double rate = 1.0;
    int outgoing, critical_lower, critical_upper;
    
    detect_outgoing_and_update_critical(arrival_times, n, lower_curve, upper_curve, rate, &outgoing, &critical_lower, &critical_upper);
    
    assert(outgoing == -1);

    printf("Test detect_outgoing_and_update_critical passed.\n");
}

void test_compute_arrival_curves() {
    compute_arrival_curves();

    assert(num_rates > 0); 

    printf("Test compute_arrival_curves passed.\n");
}

void
test_invalid_inputs()
{
    int outgoing, critical_lower, critical_upper;
    int invalid_arrival_times[] = {-10, INT_MAX, 20};
    double sigma, T;

    determine_sigma_T(invalid_arrival_times, 3, &sigma, &T);

    detect_outgoing_and_update_critical(NULL, 6, sigma, T, 1.0, &outgoing, &critical_lower, &critical_upper);

    detect_outgoing_and_update_critical(invalid_arrival_times, 3, sigma, T, 1.0, &outgoing, &critical_lower, &critical_upper);

    detect_outgoing_and_update_critical(invalid_arrival_times, -1, sigma, T, 1.0, &outgoing, &critical_lower, &critical_upper);

    detect_outgoing_and_update_critical(invalid_arrival_times, 6, sigma, T, 1.0, NULL, &critical_lower, &critical_upper);

    printf("Test invalid inputs handled correctly.\n");
}

void
test_large_dataset()
{
    int large_arrival_times[1000];

    for (int i = 0; i < 1000; i++) {
        large_arrival_times[i] = i * 10;
    }
    double sigma, T;

    determine_sigma_T(large_arrival_times, 1000, &sigma, &T);

    int outgoing, critical_lower, critical_upper;
    detect_outgoing_and_update_critical(large_arrival_times, 1000, sigma, T, 1.0, &outgoing, &critical_lower, &critical_upper);

    printf("Test large dataset handled correctly.\n");
}

void
test_edge_cases()
{
    int edge_case_times[] = {0, 1, 2, 3, 4, 5};
    double sigma, T;

    determine_sigma_T(edge_case_times, 6, &sigma, &T);

    int outgoing, critical_lower, critical_upper;

    detect_outgoing_and_update_critical(edge_case_times, 6, sigma, T, 1.0, &outgoing, &critical_lower, &critical_upper);

    printf("Test edge cases passed.\n");
}

int
main()
{
    test_detect_outgoing_and_update_critical();

    test_compute_arrival_curves();

    test_invalid_inputs();

    test_large_dataset();

    test_edge_cases();

    printf("All tests passed successfully!\n");

    return 0;
}