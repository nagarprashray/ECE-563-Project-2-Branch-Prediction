#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim_bp.h"
#include <math.h>
#include <iostream>
#include <iomanip>

using namespace std;

int main(int argc, char *argv[]) {
    char *traceFilename;           // Store the trace file name
    BranchPredictorParams predictorParams; // Struct to hold the predictor parameters
    char outcome;                  // Variable for branch outcome
    unsigned long int address;     // Variable to store the address from input file

    // Prediction counters
    int incorrectPredictions = 0;
    int totalPredictions = 0;
    float missPredictionRate = 0;

    // Bimodal predictor data
    int *bimodalArray;
    int bimodalMask;
    int bimodalSize;
    int bimodalIndex;
    predictorParams.M2 = 0;
    predictorParams.M1 = 0;
    predictorParams.K = 0;
    predictorParams.N = 0;

    // Gshare predictor data
    int *gshareArray;
    int gshareMask;
    int gshareSize;
    int gshareIndex;
    int tempIndex;
    int globalHistoryReg = 0b0;
    int tempGlobalHistory = 0b0;

    // Hybrid predictor data
    int hybridIndex;
    int hybridSize;
    int *hybridArray;

    // Branch outcomes
    char bimodalOutcome;
    char gshareOutcome;

    // Verify the number of arguments
    if (!(argc == 4 || argc == 5 || argc == 7)) {
        printf("Error: Incorrect number of inputs: %d\n", argc - 1);
        exit(EXIT_FAILURE);
    }

    predictorParams.bp_name = argv[1];

    if (strcmp(predictorParams.bp_name, "bimodal") == 0) { // Bimodal predictor
        if (argc != 4) {
            printf("Error: %s has incorrect number of inputs: %d\n", predictorParams.bp_name, argc - 1);
            exit(EXIT_FAILURE);
        }
        predictorParams.M2 = strtoul(argv[2], NULL, 10);
        traceFilename = argv[3];
        printf("COMMAND\n%s %s %lu %s\n", argv[0], predictorParams.bp_name, predictorParams.M2, traceFilename);

        // BIMODAL ARRAY INIT
        bimodalSize = pow(2, predictorParams.M2);
        bimodalArray = new int[bimodalSize];

        for (int i = 0; i < bimodalSize; i++) {
            bimodalArray[i] = 2;
        }

    } else if (strcmp(predictorParams.bp_name, "gshare") == 0) { // Gshare predictor
        if (argc != 5) {
            printf("Error: %s has incorrect number of inputs: %d\n", predictorParams.bp_name, argc - 1);
            exit(EXIT_FAILURE);
        }
        predictorParams.M1 = strtoul(argv[2], NULL, 10);
        predictorParams.N = strtoul(argv[3], NULL, 10);
        traceFilename = argv[4];
        printf("COMMAND\n%s %s %lu %lu %s\n", argv[0], predictorParams.bp_name, predictorParams.M1, predictorParams.N, traceFilename);

        // GSHARE ARRAY INIT
        gshareSize = pow(2, predictorParams.M1);
        gshareArray = new int[gshareSize];

        for (int i = 0; i < gshareSize; i++) {
            gshareArray[i] = 2;
        }

    } else if (strcmp(predictorParams.bp_name, "hybrid") == 0) { // Hybrid predictor
        if (argc != 7) {
            printf("Error: %s has incorrect number of inputs: %d\n", predictorParams.bp_name, argc - 1);
            exit(EXIT_FAILURE);
        }
        predictorParams.K = strtoul(argv[2], NULL, 10);
        predictorParams.M1 = strtoul(argv[3], NULL, 10);
        predictorParams.N = strtoul(argv[4], NULL, 10);
        predictorParams.M2 = strtoul(argv[5], NULL, 10);
        traceFilename = argv[6];
        printf("COMMAND\n%s %s %lu %lu %lu %lu %s\n", argv[0], predictorParams.bp_name, predictorParams.K, predictorParams.M1, predictorParams.N, predictorParams.M2, traceFilename);

        // HYBRID ARRAY INIT
        hybridSize = pow(2, predictorParams.K);
        hybridArray = new int[hybridSize];

        for (int i = 0; i < hybridSize; i++) {
            hybridArray[i] = 1;
        }

        // BIMODAL ARRAY INIT
        bimodalSize = pow(2, predictorParams.M2);
        bimodalArray = new int[bimodalSize];

        for (int i = 0; i < bimodalSize; i++) {
            bimodalArray[i] = 2;
        }

        // GSHARE ARRAY INIT
        gshareSize = pow(2, predictorParams.M1);
        gshareArray = new int[gshareSize];

        for (int i = 0; i < gshareSize; i++) {
            gshareArray[i] = 2;
        }

    } else {
        printf("Error: Invalid branch predictor name: %s\n", predictorParams.bp_name);
        exit(EXIT_FAILURE);
    }

    // Open the trace file
    FILE *traceFile = fopen(traceFilename, "r");
    if (traceFile == NULL) {
        printf("Error: Cannot open file %s\n", traceFilename);
        exit(EXIT_FAILURE);
    }

    char branchResult[2];
    while (fscanf(traceFile, "%lx %s", &address, branchResult) != EOF) {
        outcome = branchResult[0];
        totalPredictions++;

        /* BIMODAL PREDICTOR */
        if (strcmp(predictorParams.bp_name, "bimodal") == 0) {
            address = address >> 2;
            bimodalMask = (bimodalSize - 1);
            bimodalIndex = address & bimodalMask;

            if (outcome == 't') {
                if (bimodalArray[bimodalIndex] < 2)
                    incorrectPredictions++;

                if (bimodalArray[bimodalIndex] < 3)
                    bimodalArray[bimodalIndex]++;

            } else if (outcome == 'n') {
                if (bimodalArray[bimodalIndex] >= 2)
                    incorrectPredictions++;

                if (bimodalArray[bimodalIndex] > 0)
                    bimodalArray[bimodalIndex]--;
            }
        }

        /* GSHARE PREDICTOR */
        if (strcmp(predictorParams.bp_name, "gshare") == 0) {
            if (predictorParams.N != 0) {
                address = address >> 2;
                gshareMask = ((1 << predictorParams.M1) - 1);
                tempIndex = address & gshareMask;
                tempGlobalHistory = globalHistoryReg << (predictorParams.M1 - predictorParams.N);
                gshareIndex = tempGlobalHistory ^ tempIndex;
            } else {
                address = address >> 2;
                gshareMask = (gshareSize - 1);
                gshareIndex = address & gshareMask;
            }

            if (outcome == 't') {
                if (gshareArray[gshareIndex] < 2)
                    incorrectPredictions++;

                if (gshareArray[gshareIndex] < 3)
                    gshareArray[gshareIndex]++;

                globalHistoryReg = globalHistoryReg >> 1;
                globalHistoryReg |= (1 << (predictorParams.N - 1));
            } else if (outcome == 'n') {
                if (gshareArray[gshareIndex] >= 2)
                    incorrectPredictions++;

                if (gshareArray[gshareIndex] > 0)
                    gshareArray[gshareIndex]--;

                globalHistoryReg = globalHistoryReg >> 1;
            }
        }

        /* HYBRID PREDICTOR */
        if (strcmp(predictorParams.bp_name, "hybrid") == 0) {
            address = address >> 2;
            hybridIndex = address & ((1 << predictorParams.K) - 1);
            bimodalMask = (bimodalSize - 1);
            bimodalIndex = address & bimodalMask;

            if (predictorParams.N != 0) {
                gshareMask = ((1 << predictorParams.M1) - 1);
                tempIndex = address & gshareMask;
                tempGlobalHistory = globalHistoryReg << (predictorParams.M1 - predictorParams.N);
                gshareIndex = tempGlobalHistory ^ tempIndex;
            } else {
                gshareMask = (gshareSize - 1);
                gshareIndex = address & gshareMask;
            }

            bimodalOutcome = bimodalArray[bimodalIndex] >= 2 ? 't' : 'n';
            gshareOutcome = gshareArray[gshareIndex] >= 2 ? 't' : 'n';

            if (hybridArray[hybridIndex] >= 2) {
                if (outcome == 't') {
                    if (gshareOutcome == 'n') incorrectPredictions++;
                    if (gshareArray[gshareIndex] < 3) gshareArray[gshareIndex]++;
                } else if (outcome == 'n') {
                    if (gshareOutcome == 't') incorrectPredictions++;
                    if (gshareArray[gshareIndex] > 0) gshareArray[gshareIndex]--;
                }
            } else {
                if (outcome == 't') {
                    if (bimodalOutcome == 'n') incorrectPredictions++;
                    if (bimodalArray[bimodalIndex] < 3) bimodalArray[bimodalIndex]++;
                } else if (outcome == 'n') {
                    if (bimodalOutcome == 't') incorrectPredictions++;
                    if (bimodalArray[bimodalIndex] > 0) bimodalArray[bimodalIndex]--;
                }
            }

            globalHistoryReg = (outcome == 't') ? (globalHistoryReg >> 1 | (1 << (predictorParams.N - 1))) : (globalHistoryReg >> 1);

            if ((outcome == gshareOutcome) && (outcome != bimodalOutcome)) {
                if (hybridArray[hybridIndex] < 3) hybridArray[hybridIndex]++;
            } else if ((outcome != gshareOutcome) && (outcome == bimodalOutcome)) {
                if (hybridArray[hybridIndex] > 0) hybridArray[hybridIndex]--;
            }
        }
    }

    // Display the output
    cout << " OUTPUT" << endl;
    cout << "  number of predictions:    " << totalPredictions << endl;
    cout << "  number of mispredictions: " << incorrectPredictions << endl;
    missPredictionRate = ((float) incorrectPredictions / totalPredictions) * 100;
    printf("  misprediction rate:        %0.2f%% \n", missPredictionRate);

    if (strcmp(predictorParams.bp_name, "bimodal") == 0) { // BIMODAL
        cout << "FINAL BIMODAL CONTENTS" << endl;
        for (int i = 0; i < bimodalSize; i++) {
            cout << i << "       " << bimodalArray[i] << endl;
        }
    } else if (strcmp(predictorParams.bp_name, "gshare") == 0) { // GSHARE
        cout << "FINAL GSHARE CONTENTS" << endl;
        for (int i = 0; i < gshareSize; i++) {
            cout << i << "       " << gshareArray[i] << endl;
        }
    } else if (strcmp(predictorParams.bp_name, "hybrid") == 0) { // HYBRID
        cout << "FINAL CHOOSER CONTENTS" << endl;
        for (int i = 0; i < hybridSize; i++) {
            cout << i << "       " << hybridArray[i] << endl;
        }
        cout << "FINAL GSHARE CONTENTS" << endl;
        for (int i = 0; i < gshareSize; i++) {
            cout << i << "       " << gshareArray[i] << endl;
        }
        cout << "FINAL BIMODAL CONTENTS" << endl;
        for (int i = 0; i < bimodalSize; i++) {
            cout << i << "       " << bimodalArray[i] << endl;
        }
    }

    return 0;
}
