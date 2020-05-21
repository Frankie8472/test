/*
    Benchmarks for the various Baum Welch algorithm implementations
    Structure code such that it produces output results we can copy-paste into latex plots (minimize overhead)

    -----------------------------------------------------------------------------------

    Spring 2020
    Advanced Systems Lab (How to Write Fast Numerical Code)
    Semester Project: Baum-Welch algorithm

    Authors
    Josua Cantieni, Franz Knobel, Cheuk Yu Chan, Ramon Witschi
    ETH Computer Science MSc, Computer Science Department ETH Zurich

    -----------------------------------------------------------------------------------
*/

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <climits>
#include <vector>
#include <set>
#include <getopt.h>
// custom files for the project
#include "tsc_x86.h"
#include "helper_utilities.h"
#include "common.h"
#include <random>

#define NUM_RUNS 100
#define CYCLES_REQUIRED 1e8
#define FREQUENCY 2.2e9
#define CALIBRATE 1
#define REP 20

/*
Cost analysis (add, mul and div is one flop)

forward: (1 add + 1 mul)*K*N²*T + (1 add + 2 mul)*K*N*T + (1 add + 2 mul)*K*N + (1 div)*K*T + (1 div)*K
backward: (1 add + 2 mul)*K*N²*(T-1) + (1 mul)*K*N*(T-1)
compute gamma: (1 div + 1 mul)*K*N*T + (1 add)*K*N*(T-1)
compute sigma: (1 add + 3 mul)*K*N²*(T-1)
update init: (1 add)*K*N + (1 div)*N
update trans: (2 adds)*K*N² + (1 div)*N²
update emit: (2 adds)*N*M*K + (1 add)*K*N*T + (1 add)*K*N + (1 div)*N*M
neg_log_likelihood_sum: (1 add)*K*T

total: (1 add + 1 mul)*K*N²*T + (2 add + 5 mul)*K*N²*(T-1) + (2 adds)*K*N² + (1 div)*N² + (2 add + 3 mul + div)*K*N*T + (1 add + 1 mul)*K*N*(T-1)
    + (3 add + 2 mul)*K*N + (1 div)*K + (2 adds)*N*M*K + (1 div + 1 add)*K*T + (1 div)*N + (1 div)*N*M

    ≈ 9*T*K*N² - 5*K*N² + N² + 8*T*K*N + 3*K*N + K + 2*K*N*M + 2*T*K + N + N*M
*/

size_t flops;

// adjust max_iterations if it's too slow
size_t max_iterations = 100;

void perf_test(compute_bw_func func, const BWdata& bw) {
    double cycles = 0.;
    size_t num_runs = 1;
    double perf;
    myInt64 start, end;
    std::vector<const BWdata *> bw_data;

#if CALIBRATE
    double multiplier = 1;
    // Warm-up phase: we determine a number of executions that allows
    // the code to be executed for at least CYCLES_REQUIRED cycles.
    // This helps excluding timing overhead when measuring small runtimes.

    do {
        num_runs = num_runs * multiplier;
        for(size_t i = 0; i < num_runs; i++) {
            bw_data.push_back(new BWdata(bw));
        }

        start = start_tsc();
        for (size_t i = 0; i < num_runs; i++) {
            func(*bw_data.at(i));
        }

        end = stop_tsc(start);

        for(size_t i = 0; i < num_runs; i++) {
            delete bw_data.at(i);
        }
        bw_data.clear();
        cycles = (double)end;
        multiplier = (CYCLES_REQUIRED) / (cycles);

    } while (multiplier > 2);

#endif
    // Actual performance measurements repeated REP times.

    double total_cycles = 0;
    size_t iter;
    size_t total_iter = 0;
    for (size_t j = 0; j < REP; j++) {
        iter = 0;
        // Create all copies for all runs of the function
        for(size_t i = 0; i < num_runs; i++) {
            bw_data.push_back(new BWdata(bw));
        }
        
        // Measure function
        start = start_tsc();
        for (size_t i = 0; i < num_runs; ++i) {
            iter += func(*bw_data.at(i));
        }
        end = stop_tsc(start);
        
        // Clean up all copies
        for(size_t i = 0; i < num_runs; i++) {
            delete bw_data.at(i);
        }
        bw_data.clear();
        
        cycles = ((double)end) / num_runs;
        total_cycles += cycles;
        iter /= num_runs;
        total_iter += iter;
    }
    total_cycles /= REP;
    total_iter /= REP;

    cycles = total_cycles;
    iter = total_iter;
    perf =  round((100.0 * max_iterations*flops) / cycles) / 100.0;

    printf("Total iterations: %ld\n", max_iterations);
    if (iter == 0){
        printf("\x1b[1;36mWarning:\x1b[0m has not converged within the maximum iterations\n");
    } else {
        printf("Iterations to converge: %ld\n", iter);
    }
    printf("Flops: %zu\n", max_iterations*flops);
    printf("Cycles: %f\n", round(cycles));
    printf("Performance: %f\n", perf);
}

static struct option arg_options[] = {
        {"test", no_argument, NULL, 't'}, 
        {"only", required_argument, NULL, 'o'},
        {"list", no_argument, NULL, 'l'},
        {"max-iterations", required_argument, NULL, 1},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

int main(int argc, char **argv) {

    // randomize seed
    srand(time(NULL));

    std::set<std::string> sel_impl;
    std::string arg;

    // no need for variable size randomness in benchmarks
    // NOTE: Please choose a nonzero multiple of 16 (and 32 for T)
    const size_t K = 16;
    const size_t N = 16;
    const size_t M = 16;
    const size_t T = 32;

    // Parse arguments
    while(true){
        int c = getopt_long(argc, argv, "tho:", arg_options, NULL);

        if (c == -1)
            break;

        switch(c){
            case 'o':
                arg = optarg;
                sel_impl.insert(arg);
                break;
            case 't':
                printf("This option is not yet implemented.\n");
                printf(":(\n");
                exit(-2);
            case 'l':
                printf("Registered functions:\n");
                FuncRegister::printRegisteredFuncs();
                exit(0);
            case 1:
                max_iterations = atoi(optarg);
                if(max_iterations % 4 != 0){
                    printf("Error: max_iterations needs to be divisable by 4\n");
                    exit(-1);
                }
                break;
            case 'h':
                printf("Usage: %s [OPTIONS]\n", argv[0]);
                printf("Benchmarks the registered implementations against the registered baseline.\n\n");
                printf("Options:\n");
                printf("  -h, --help\t\t\tPrints this message and exits\n");
                printf("  -t, --test\t\t\tPerform test that can be used for the report (Not yet implemented)\n");
                printf("  -o, --only <name>\t\tOnly execute the implementation with the given name (case-sensitive). "
                                 "\n  \t\t\t\t Can occur multiple times and is compatible with --test. The baseline is"
                                 "\n  \t\t\t\t always run.\n");
                printf("      --list\t\t\tLists all available implementations and exits\n");
                printf("      --max-iterations <value>\tSets the max-iteration to a value\n");
                exit(0);
            case '?':
                exit(-1);
            default:
                printf("argument not supported\n");
                break;
        }

    }

    printf("Benchmarking with K = %zu, N = %zu, M = %zu, T = %zu and max_iterations = %zu\n", K, N, M, T, max_iterations);

    flops = 9*T*K*N*N - 5*K*N*N + N*N + 8*T*K*N + 3*K*N + K + 2*K*N*M + 2*T*K + N + N*M;

    const BWdata& bw = *new BWdata(K, N, M, T, max_iterations);
    initialize_random(bw);
    printf("Running: %s\n", FuncRegister::baseline_name.c_str());
    perf_test(FuncRegister::baseline_func, bw);
    printf("\n");
    for(size_t i = 0; i < FuncRegister::size(); i++){
        if(sel_impl.empty() || sel_impl.find(FuncRegister::funcs->at(i).name) != sel_impl.end()){

            // Hacky but it works: Transpose emit_prob
            if(FuncRegister::funcs->at(i).transpose_emit_prob){
                double *new_emit_prob = (double *)malloc(bw.N*bw.M * sizeof(double));
                transpose_matrix(new_emit_prob, bw.emit_prob, bw.N, bw.M);
                memcpy(bw.emit_prob, new_emit_prob, bw.N*bw.M * sizeof(double));
                free(new_emit_prob);
            }

            printf("Running: %s: %s\n", FuncRegister::funcs->at(i).name.c_str(), FuncRegister::funcs->at(i).description.c_str());
            perf_test(FuncRegister::funcs->at(i).func, bw);
            printf("\n");
        }
    }
    delete &bw;
}
