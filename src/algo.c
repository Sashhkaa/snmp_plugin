#include "agent.h"

void
update_critical(double rho, CriticalPoints *c,
                int n, ArrivalHistory *history)
{
    double condition = rho * (history->arrival_times[n] - 
                              history->arrival_times[c->lower]) + 
                       c->lower;
    
    if (n < condition) {
        c->lower = n;
    } else if (n > condition) {
        c->upper = n;
    }
}

bool
is_lower_constrained(double T, double rho, int n, int c,
                    ArrivalHistory *history)
{
    return n >= rho * (history->arrival_times[n] -
                       history->arrival_times[c] - T) + c;
}

bool
is_upper_constrained(double sigma, double rho,
                     int n, int c, ArrivalHistory *history)
{
    return n <= rho * (history->arrival_times[n] -
                       history->arrival_times[c]) +
                sigma + c;
}

double
compute_rho0(double *xn, int length)
{
    if (length < 2)
        return 1.0;
    return (double)(xn[length - 1] - xn[0]) / (length - 1);
}

void
compute_rates(ArrivalHistory *history, double rho0)
{
    double rho = rho0;
    int n = history->count;

    for (int i = 1; i < n; i++) {
        if (!is_lower_constrained(T, rho, i, c.lower, history)) {
            rho = (double)(i - c.lower) / (history->arrival_times[i] -
                                           history->arrival_times[c.lower]);
            c.lower = i;
            c.upper = i;

            rates[num_rates++] = rho;
        } else if (!is_upper_constrained(sigma, rho, i, c.upper, history)) {
            rho = (double)(i - c.upper) / (history->arrival_times[i] -
                                           history->arrival_times[c.upper]);
            c.lower = i;
            c.upper = i;

            rates[num_rates++] = rho;
        } else {
            update_critical(rho, &c, i, history);
        }
    }
}