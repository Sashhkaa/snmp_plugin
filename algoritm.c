#include "algoritm.h"

static int arrival_times[MAX_MESSAGES] = {0, 10, 20, 30, 50, 70};
static int n = 6;
static int sigma = 1, T = 10;
static double rates[MAX_MESSAGES];
static int num_rates = 0;

void
compute_arrival_curves(void)
{
    pthread_mutex_lock(&lock);
    if (n <= 1 || arrival_times[1] == 0) {
        fprintf(stderr, "Invalid arrival times or insufficient data for curve computation\n");
        pthread_mutex_unlock(&lock);
        return;
    }
    
    int m = 0;
    double rate = 1.0 / arrival_times[1];
    int critical_lower = 0, critical_upper = 0;
    num_rates = 0;

    for (int i = 1; i < n; i++) {
        int outgoing;
        detect_outgoing_and_update_critical(arrival_times, n, sigma, T, rate, &outgoing, &critical_lower, &critical_upper);
        
        if (outgoing != -1) {
            if (arrival_times[outgoing] - arrival_times[critical_lower] == 0) {
                fprintf(stderr, "Division by zero detected in rate computation\n");
                pthread_mutex_unlock(&lock);
                return;
            }
            
            rate = (double)(arrival_times[outgoing] - arrival_times[critical_lower]) / (outgoing - critical_lower);
            rates[num_rates++] = rate;
            m = outgoing;
        }

    }
    pthread_mutex_unlock(&lock);
}

void
detect_outgoing_and_update_critical(int *arrival_times,
                                    int n, double lower_curve,
                                    double upper_curve, double rate,
                                    int *outgoing, int *critical_lower,
                                    int *critical_upper)
{
    if (!arrival_times || n <= 0 || !outgoing || !critical_lower || !critical_upper) {
        fprintf(stderr, "Invalid input parameters to detect_outgoing_and_update_critical\n");
        return;
    }
    
    int m = 0;
    *critical_lower = m;
    *critical_upper = m;
    *outgoing = -1;

    for (int i = 1; i < n; i++) {
        if (arrival_times[i] < 0 || arrival_times[i] > INT_MAX) {
            fprintf(stderr, "Invalid arrival time at index %d\n", i);
            continue;
        }

        if (arrival_times[i] < lower_curve || arrival_times[i] > upper_curve) {
            *outgoing = i;
            break;
        }

        if (i * rate - arrival_times[i] > (*critical_lower) * rate - arrival_times[*critical_lower]) {
            *critical_lower = i;
        }
        
        if (i * rate - arrival_times[i] < (*critical_upper) * rate - arrival_times[*critical_upper]) {
            *critical_upper = i;
        }
    }
}