#ifndef SIM_BP_H
#define SIM_BP_H

struct BranchPredictorParams {
    unsigned long int K;  // Chooser bits
    unsigned long int M1; // Gshare bits
    unsigned long int M2; // Bimodal bits
    unsigned long int N;  // Global history bits
    char* bp_name;        // Predictor type
};

// Add any additional structures or enums required for the implementation

#endif // SIM_BP_REWRITE_H
