/*
    Helper Utilities
    Throw all useful functions that may or may not be used more than once in here

    -----------------------------------------------------------------------------------

    Spring 2020
    Advanced Systems Lab (How to Write Fast Numerical Code)
    Semester Project: Baum-Welch algorithm

    Authors
    Josua Cantieni, Franz Knobel, Cheuk Yu Chan, Ramon Witschi
    ETH Computer Science MSc, Computer Science Department ETH Zurich

    -----------------------------------------------------------------------------------
*/

#include <stdlib.h>
#include <stdio.h>
#include <random>


void initialize_uar(
    const unsigned int K,  // number of observation sequences / training datasets
    const unsigned int N,  // number of hidden state variables
    const unsigned int M,  // number of distinct observations
    const unsigned int T,  // number of time steps
    unsigned int* const observations,
    double* const init_prob,
    double* const trans_prob,
    double* const emit_prob
) {

    // uniform at random set init_prob and trans_prob
    for (int n0 = 0; n0 < N; n0++) {
        init_prob[n0] = 1.0/N;
        for (int n1 = 0; n1 < N; n1++) {
            trans_prob[n0*N + n1] = 1.0/N;
        }
    }

    // uniform at random set emit_prob
    for (int n = 0; n < N; n++) {
        for (int m = 0; m < M; m++) {
            emit_prob[n*M + m] = 1.0/M;
        }
    }

    // uniform at random set observations
    // (well, not really u.a.r. but let's pretend)
    for (int k = 0; k < K; k++) {
        for (int t = 0; t < T; t++) {
            observations[k*T + t] = t;
        }
    }
}


void initialize_random(
    const unsigned int K,  // number of observation sequences / training datasets
    const unsigned int N,  // number of hidden state variables
    const unsigned int M,  // number of distinct observations
    const unsigned int T,  // number of time steps
    unsigned int* const observations,
    double* const init_prob,
    double* const trans_prob,
    double* const emit_prob
) {

    double init_sum;
    double trans_sum;
    double emit_sum;

    // randomly initialized init_prob
    init_sum = 0.0;
    while(init_sum == 0.0){
        for (int n = 0; n < N; n++) {
            init_prob[n] = rand();
            init_sum += init_prob[n];
        }
    }

    // the array init_prob must sum to 1.0
    for (int n = 0; n < N; n++) {
        init_prob[n] /= init_sum;
    }

    // randomly initialized trans_prob rows
    for (int n0 = 0; n0 < N; n0++) {
        trans_sum = 0.0;
        while(trans_sum == 0.0){
            for (int n1 = 0; n1 < N; n1++) {
                trans_prob[n0*N + n1] = rand();
                trans_sum += trans_prob[n0*N + n1];
            }
        }

        // the row trans_prob[n0*N] must sum to 1.0
        for (int n1 = 0; n1 < N; n1++) {
            trans_prob[n0*N + n1] /= trans_sum;
        }
    }

    // randomly initialized emit_prob rows
    for (int n = 0; n < N; n++) {
        emit_sum = 0.0;
        while (emit_sum == 0.0) {
            for (int m = 0; m < M; m++) {
                emit_prob[n * M + m] = rand();
                emit_sum += emit_prob[n * M + m];
            }
        }

        // the row emit_prob[n*M] must sum to 1.0
        for (int m = 0; m < M; m++) {
            emit_prob[n*M + m] /= emit_sum;
        }
    }

    // fixed observation (can be changed to e.g. all 1 for verification)
    for (int k = 0; k < K; k++) {
        for (int t = 0; t < T; t++) {
            // % T would be wrong, because the observations sequence (over time 0 <= t < T)
            // represents observations (categorical random variable) in 0 <= m < M
            observations[k*T + t] = rand() % M;
        }
    }
}

// TODO: Description
void check_and_verify(
    const unsigned int max_iterations,
    const unsigned int N,
    const unsigned int M,
    double* const init_prob,
    double* const trans_prob,
    double* const emit_prob,
    double* const neg_log_likelihoods
) {

    unsigned int errors;
    double init_sum;
    double trans_sum;
    double emit_sum;

    // check if the initial distribution sums to 1.0
    init_sum = 0.0;
    for (int n = 0; n < N; n++) init_sum += init_prob[n];
    if (1.0 - 1e-12 < init_sum && init_sum < 1.0 + 1e-12) printf("\nPASSED: init_prob sums to 1.0");
    else printf("\n1 VIOLATIONS: init_prob sums to %lf\n", init_sum);

    // check if the rows of the transition distribution sum to 1.0
    errors = 0;
    for (int n0 = 0; n0 < N; n0++) {
        trans_sum = 0.0;
        for (int n1 = 0; n1 < N; n1++) {
            trans_sum += trans_prob[n0*N + n1];
        }
        if ( ! ( 1.0 - 1e-12 < trans_sum && trans_sum < 1.0 + 1e-12 ) ) { 
            errors++;
            printf("\nFAIL: trans_prob[%d] sums to %lf", n0, trans_sum);
        }
    }
    if (errors > 0) printf("\n%d VIOLATIONS of rows in trans_prob that do not sum to 1.0\n", errors);
    else printf("\nPASSED: trans_prob rows sum to 1.0");

    // check if the rows of the emission distribution sum to 1.0
    errors = 0;
    for (int n = 0; n < N; n++) {
        emit_sum = 0.0;
        for (int m = 0; m < M; m++) {
            emit_sum += emit_prob[n*M + m];
        }
        if ( ! ( 1.0 - 1e-12 < emit_sum && emit_sum < 1.0 + 1e-12 ) ) { 
            errors++;
            printf("\nFAIL: emit_prob[%d] sums to %lf", n, emit_sum);
        }
    }
    if (errors > 0) printf("\n%d VIOLATIONS of rows in emit_prob that do not sum to 1.0\n", errors);
    else printf("\nPASSED: emit_prob rows sum to 1.0");

    // check the negative log likelihood sequence for monotonicity
    errors = 0;
    for (int iterations = 1; iterations < max_iterations; iterations++) {
        double old_nll = neg_log_likelihoods[iterations-1];
        double new_nll = neg_log_likelihoods[iterations];
        if (old_nll > new_nll + 1e-12) {
            errors++;
            printf("\n[%d]\t%lf\t > \t%lf \t(old nll > new nll)", iterations, old_nll, new_nll);
        }
    }
    if (errors > 0) printf("\n%d VIOLATIONS of the monotonicity of the negative log likelihood\n", errors);
    else printf("\nPASSED: monotonocity of the negative log likelihood\n");
}


void print_states(
    const unsigned int N,
    const unsigned int M,
    const unsigned int T,
    double* const init_prob,
    double* const trans_prob,
    double* const emit_prob
    ) {

    printf("\nInitialization probabilities:\n");
    for(int n = 0; n < N; n++) {
        printf("Pr[X_1 = %d] = %f\n", n+1, init_prob[n]);
    }

    printf("\nTransition probabilities:\n");
    for(int n0 = 0; n0 < N; n0++) {
        for(int n1 = 0; n1 < N; n1++) {
            printf("Pr[X_t = %d | X_(t-1) = %d ] = %f\n", n1+1, n0+1, trans_prob[n0*N + n1]);
        }
    }

    printf("\nEmission probabilities:\n");
    for(int n = 0; n < N; n++) {
        for(int m = 0; m < M; m++) {
            printf("Pr[Y_t = %d | X_t = %d] = %f\n", m+1, n+1, emit_prob[n*M + m]);
        }
    }
    printf("\n");
}