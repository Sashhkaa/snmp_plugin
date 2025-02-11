#include <stdio.h>
#include <stdlib.h>

void compute_arrival_curves(void);
void detect_outgoing_and_update_critical(int *arrival_times,
                                         int n, double lower_curve,
                                         double upper_curve, double rate,
                                         int *outgoing, int *critical_lower,
                                         int *critical_upper)
