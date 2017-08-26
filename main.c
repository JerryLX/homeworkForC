/***************************************************************************
 *
 *   File        : main.c
 *   Student Id  : <INSERT STUDENT ID HERE>
 *   Name        : <INSERT STUDENT NAME HERE>
 *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>
#include "tasks.h"

int main(int argc, char *argv[]) {
	
	/* TODO: Parse Command Line Arguments */
	char* flow_file = NULL;
	int resolution = 0;
    struct timeval timemark1,timemark2,timemark3,timemark4,timemark5;
    float elapsed1,elapsed2,elapsed3,elapsed4;
    
    if(argc < 3){
        printf("USAGE: %s <flow_file name> <resolution>\n", argv[0]);
        return (EXIT_FAILURE);
    }
    flow_file = argv[1];
    resolution = atoi(argv[2]);

    gettimeofday(&timemark1,NULL);
    maxveldiff(flow_file);
	
    gettimeofday(&timemark2,NULL);
    elapsed1 = (timemark2.tv_sec - timemark1.tv_sec) * 1000.0;
    printf("TASK 1: %.2f milliseconds\n", elapsed1);

    coarsegrid(flow_file, resolution);
	
    gettimeofday(&timemark3,NULL);
    elapsed2 = (timemark3.tv_sec - timemark2.tv_sec) * 1000.0;
    printf("TASK 2: %.2f milliseconds\n", elapsed2);
	
    velstat(flow_file);
    gettimeofday(&timemark4,NULL);
    elapsed3 = (timemark4.tv_sec - timemark3.tv_sec) * 1000.0;
    printf("TASK 3: %.2f milliseconds\n", elapsed3);
	
    wakevis(flow_file);
    
    gettimeofday(&timemark5,NULL);
    elapsed4 = (timemark5.tv_sec - timemark4.tv_sec) * 1000.0;
    printf("TASK 4: %.2f milliseconds\n", elapsed4);
	return (EXIT_SUCCESS);
}
