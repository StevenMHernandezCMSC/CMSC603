#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <float.h>
#include <math.h>
#include <iostream>
#include "libarff/arff_parser.h"
#include "libarff/arff_data.h"

using namespace std;

#define K_VALUE 5

float euclideanDistance(ArffInstance *x, ArffInstance *xi) {
    float sum = 0;

    /*
     * for each attribute (not counting the class)
     */
    for (int a = 0; a < x->size() - 1; a++) {
        sum += pow(x->get(a)->operator float() - xi->get(a)->operator float(), 2);
    }

    return sqrt(sum);
}

int* KNN(ArffData* dataset, int k_value)
{
    int* predictions = (int*)malloc(dataset->num_instances() * sizeof(int));

    /*
     * Find the maximum number of classes in the data.
     * This is used to determine the highest voted class per data point.
     */
    int max_class_number = 0;

    for (int i = 0; i < dataset->num_instances(); i++) {
        int classValue = dataset->get_instance(i)->get(dataset->num_attributes() - 1)->operator int32();

        if (classValue > max_class_number) {
            max_class_number = classValue;
        }
    }

    /*
     * For all data in the dataset
     */
    for (int i = 0; i < dataset->num_instances(); i++) {
        /*
         * Create arrays to store the k nearest neighbors (storing their class and distance
         */
        int neighbors_class[k_value];
        float neighbors_distance[k_value];

        for (int n = 0; n < k_value; n++) {
            neighbors_class[n] = 0;
            neighbors_distance[n] = FLT_MAX;
        }

        /*
         * compare to all other data in the dataset
         */
        for (int j = 0; j < dataset->num_instances(); j++) {
            // do not compare to itself
            if (i != j) {
                int classValue = dataset->get_instance(j)->get(dataset->num_attributes() - 1)->operator int32();
                float distance = euclideanDistance(dataset->get_instance(i), dataset->get_instance(j));

                // sort into neighbors array
                for (int n = 0; n < k_value; n++) {
                    if (distance < neighbors_distance[n]) {
                        int tempClassValue = neighbors_class[n];
                        float tempClassDistance = neighbors_distance[n];

                        neighbors_class[n] = classValue;
                        neighbors_distance[n] = distance;

                        classValue = tempClassValue;
                        distance = tempClassDistance;
                    }
                }
            }
        }

        // count votes
        int *votes = (int*)calloc((max_class_number + 1), sizeof(int));

        // for each voted value
        for (int n = 0; n < k_value; n++) {
            votes[neighbors_class[n]] += 1;
        }

        int max_class = 0;

        for (int v = 0; v < max_class_number + 1; v++) {
            if (votes[max_class] < votes[v]) {
                max_class = v;
            }
        }

        free(votes);

        predictions[i] = max_class;
    }

    return predictions;
}

int* computeConfusionMatrix(int* predictions, ArffData* dataset)
{
    int* confusionMatrix = (int*)calloc(dataset->num_classes() * dataset->num_classes(), sizeof(int)); // matriz size numberClasses x numberClasses

    for(int i = 0; i < dataset->num_instances(); i++) // for each instance compare the true class and predicted class
    {
        int trueClass = dataset->get_instance(i)->get(dataset->num_attributes() - 1)->operator int32();
        int predictedClass = predictions[i];

        confusionMatrix[trueClass*dataset->num_classes() + predictedClass]++;
    }

    return confusionMatrix;
}

float computeAccuracy(int* confusionMatrix, ArffData* dataset)
{
    int successfulPredictions = 0;

    for(int i = 0; i < dataset->num_classes(); i++)
    {
        successfulPredictions += confusionMatrix[i*dataset->num_classes() + i]; // elements in the diagonal are correct predictions
    }

    return successfulPredictions / (float) dataset->num_instances();
}

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        cout << "Usage: ./main datasets/datasetFile.arff" << endl;
        exit(0);
    }

    ArffParser parser(argv[1]);
    ArffData *dataset = parser.parse();
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    int* predictions = KNN(dataset, K_VALUE);
    int* confusionMatrix = computeConfusionMatrix(predictions, dataset);
    float accuracy = computeAccuracy(confusionMatrix, dataset);

    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    uint64_t diff = (1000000000L * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec) / 1e6;

    printf("The KNN classifier for %lu instances required %llu ms CPU time. Accuracy was %.4f\n", dataset->num_instances(), (long long unsigned int) diff, accuracy);
}
