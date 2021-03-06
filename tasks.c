/***************************************************************************
 *
 *   File        : tasks.c
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

#define FLT_MIN 1.175494351e-38F
#define FLT_MAX 3.402823466e+38F

#define MAX_BUF_LEN 2048
#define MAX_DATA_SIZE 300000

#define XMIN 10.0
#define XMAX 70.0
#define YMIN -20.0
#define YMAX 20.0
#define WIDTH 60.0
#define HEIGHT 40.0
#define NUMOFTHRESHOLD 7
#define FIRSTTHRESHOLD 0.5
#define STEP 5
#define THRESHOLD 0.03

#define OUTPUT1 "task1.csv"
#define OUTPUT2 "task2.csv"
#define OUTPUT3 "task3.csv"
#define OUTPUT4 "task4_1.csv"


/*
 * Struct for FlowPoint
 */
struct FlowPoint{
    float x,y,u,v;
};

struct Grid{
    float x,y,u,v;
    float score;
};

void* safe_malloc(size_t num_bytes)
{
    void* ptr = malloc(num_bytes);
    if (ptr == NULL) {
            printf("ERROR: malloc(%lu)\n", num_bytes);
            exit(EXIT_FAILURE);
    }
    return ptr;
}

FILE* safe_fopen(const char* path, const char* mode)
{
    FILE* fp = fopen(path, mode);
    if (fp == NULL) {
        printf("ERROR: %s not exist!\n",path);
        exit(EXIT_FAILURE);
    }
    return fp;
}

int sort_by_x(const void *a, const void *b){
    return (((struct FlowPoint *)a)->x > ((struct FlowPoint *)b)->x); 
}

int sort_by_y(const void *a, const void *b){
    return (((struct FlowPoint *)a)->y > ((struct FlowPoint *)b)->y); 
}

int sort_by_score(const void *a, const void *b){
    return (((struct Grid *)a)->score < ((struct Grid *)b)->score); 
}

void maxveldiff(const char* flow_file)
{
    FILE *fp, *fpw;
    struct FlowPoint *data;
    char buf[MAX_BUF_LEN];
    int index = 0,index_vmax,index_vmin,index_umax,index_umin;
    float vmax = FLT_MIN, vmin = FLT_MAX;
    float umax = FLT_MIN, umin = FLT_MAX;

    fp = safe_fopen(flow_file, "r");
    data = safe_malloc(sizeof(struct FlowPoint) * MAX_DATA_SIZE);
 
    /* read data */
    fgets(buf, MAX_BUF_LEN,fp); //skip first line
    while(fscanf(fp,"%f,%f,%f,%f\n",&data[index].x,&data[index].y,&data[index].u,&data[index].v) == 4) {
        if(data[index].x <= 20){
            index++;
            continue;
        }
        if(data[index].v > vmax){
            vmax = data[index].v;
            index_vmax = index;
        }
        if(data[index].v < vmin){
            vmin = data[index].v;
            index_vmin = index;
        }
        if(data[index].u > umax){
            umax = data[index].u;
            index_umax = index;
        }
        if(data[index].u < umin){
            umin = data[index].u;
            index_umin = index;
        }
        
        index++;
    }
    fclose(fp);
    
    /* write output */
    fpw = safe_fopen(OUTPUT1,"w");
    fprintf(fpw, "x,y,u,v\n");
    fprintf(fpw, "%.6f,%.6f,%.6f,%.6f\n",data[index_umax].x,data[index_umax].y,data[index_umax].u,data[index_umax].v);
    fprintf(fpw, "%.6f,%.6f,%.6f,%.6f\n",data[index_umin].x,data[index_umin].y,data[index_umin].u,data[index_umin].v);
    fprintf(fpw, "%.6f,%.6f,%.6f,%.6f\n",data[index_vmax].x,data[index_vmax].y,data[index_vmax].u,data[index_vmax].v);
    fprintf(fpw, "%.6f,%.6f,%.6f,%.6f\n",data[index_vmin].x,data[index_vmin].y,data[index_vmin].u,data[index_vmin].v);
    fclose(fpw);
    free(data);
}


/**
 * This function compute average value and score
 * in one cell.
 */
void coarsegrid_on_y(struct FlowPoint *data, int begin, int end, struct Grid* grid){
    int i;
    float xsum=0,ysum=0,usum=0,vsum=0;
    if(begin == end){
        grid->x=0;
        grid->y=0;
        grid->u=0;
        grid->v=0;
        grid->score=0;
        return;
    }
    
    for(i=begin;i<end;i++){
        xsum+=data[i].x;
        ysum+=data[i].y;
        usum+=data[i].u;
        vsum+=data[i].v;
    }
    grid->x = xsum/(end-begin);
    grid->y = ysum/(end-begin);
    grid->u = usum/(end-begin);
    grid->v = vsum/(end-begin);
    grid->score = 100 * sqrt(pow(grid->u,2)+pow(grid->v,2))/sqrt(pow(grid->x,2)+pow(grid->y,2));
}

/**
 * This function compute average value and score
 * for all cells in one line.
 */
void coarsegrid_on_x(int resolution, struct FlowPoint *data, int begin, int end, struct Grid* grids){
    int i, ybegin, yend;
    float height_cell = HEIGHT / resolution;
    
    qsort(&data[begin],end-begin,sizeof(struct FlowPoint),sort_by_y);
    for(i=1;i<=resolution;i++){
        double cur_y = YMIN + (i-1)*height_cell; //lower bound of y
        double next_y = YMIN + i*height_cell;  //upper bound of y
        if(i == resolution)
            next_y = YMAX+1;

        for(ybegin=begin;ybegin<end;ybegin++){
            if(data[ybegin].y >= cur_y) break;
        }

        for(yend=ybegin;yend<end;yend++){
            if(data[yend].y > next_y)
                break;
        }
        coarsegrid_on_y(data,ybegin,yend,grids+(i-1));
    }    
}

void coarsegrid(const char* flow_file, int resolution)
{
    FILE *fp, *fpw;
    struct Grid *grids;
    char buf[MAX_BUF_LEN];
    int i, num = pow(resolution,2);
    float x,y,u,v;
    int *count;
    fp = safe_fopen(flow_file, "r");

    grids = safe_malloc(sizeof(struct Grid) * num);
    count = safe_malloc(sizeof(int) * num);
    memset(grids,0,sizeof(struct Grid) * num);
    memset(count,0,sizeof(int)*num);
    /* read data */
    fgets(buf, MAX_BUF_LEN,fp); //skip first line
    while(fscanf(fp,"%f,%f,%f,%f\n",&x,&y,&u,&v) == 4) {
        int index_x = (x-XMIN)/WIDTH*resolution;
        int index_y = (y-YMIN)/HEIGHT*resolution;
        if(index_x == resolution)
            index_x--;
        if(index_y == resolution)
            index_y--;

        grids[index_x * resolution + index_y].x += x;
        grids[index_x * resolution + index_y].y += y;
        grids[index_x * resolution + index_y].u += u;
        grids[index_x * resolution + index_y].v += v;
        count[index_x * resolution + index_y]++;
    }
    for(i=0;i<num;i++){
        grids[i].x = grids[i].x/count[i];
        grids[i].y = grids[i].y/count[i];
        grids[i].u = grids[i].u/count[i];
        grids[i].v = grids[i].v/count[i];
        grids[i].score = 100 * sqrt(pow(grids[i].u,2)+pow(grids[i].v,2))/sqrt(pow(grids[i].x,2)+pow(grids[i].y,2));
    }
    fclose(fp);
    /* sort result by score */
    qsort(grids,num,sizeof(struct Grid),sort_by_score);
    
    /* write output */
    fpw = safe_fopen(OUTPUT2,"w");
    fprintf(fpw, "x,y,u,v,S\n");
    for(i=0;i<num;i++){
        fprintf(fpw, "%.6f,%.6f,%.6f,%.6f,%.6f\n",grids[i].x,grids[i].y,grids[i].u,grids[i].v,grids[i].score);
    }
    fclose(fpw);
    free(grids);
    free(count);
}

void coarsegrid_bak(const char* flow_file, int resolution)
{
    FILE *fp, *fpw;
    struct FlowPoint *data;
    struct Grid *grids;
    char buf[MAX_BUF_LEN];
    int index = 0, i, start=0, end=0, num = pow(resolution,2);
    float width_cell = WIDTH / resolution;

    fp = safe_fopen(flow_file, "r");
    data = safe_malloc(sizeof(struct FlowPoint) * MAX_DATA_SIZE);

    /* read data */
    fgets(buf, MAX_BUF_LEN,fp); //skip first line
    while(fscanf(fp,"%f,%f,%f,%f\n",&data[index].x,&data[index].y,&data[index].u,&data[index].v) == 4) {
        index++;
    }
    fclose(fp);
    
    grids = safe_malloc(sizeof(struct Grid) * num);
    
    /* sort data by x*/
    qsort(data,index,sizeof(struct FlowPoint),sort_by_x);
    for(i=1;i<=resolution;i++){
        double cur_x = XMIN + (i-1)*width_cell; //lower bound of x
        double next_x = XMIN + i*width_cell;  //upper bound of x
        if(i == resolution)
            next_x = XMAX+1;
        
        for(start=0;start<index;start++){
            if(data[start].x >= cur_x) 
                break;
        }
        for(end=start;end<index;end++){
            if(data[end].x > next_x)
                break;
        }
        coarsegrid_on_x(resolution,data,start,end,grids+(i-1)*resolution);
    }
    
    free(data);

    /* sort result by score */
    qsort(grids,num,sizeof(struct Grid),sort_by_score);
    
    /* write output */
    fpw = safe_fopen(OUTPUT2,"w");
    fprintf(fpw, "x,y,u,v,S\n");
    for(i=0;i<num;i++){
        fprintf(fpw, "%.6f,%.6f,%.6f,%.6f,%.6f\n",grids[i].x,grids[i].y,grids[i].u,grids[i].v,grids[i].score);
    }
    fclose(fpw);
    free(grids);
}

void velstat(const char* flow_file)
{
    FILE *fp, *fpw;
    char buf[MAX_BUF_LEN];
    int index = 0, i;
    float x,y,u,v, start = FIRSTTHRESHOLD;
    int num[NUMOFTHRESHOLD];

    memset(num,0,NUMOFTHRESHOLD*sizeof(int));
 
    /* read data*/
    fp = safe_fopen(flow_file, "r");
    fgets(buf, MAX_BUF_LEN,fp); //skip first line
    while(fscanf(fp,"%f,%f,%f,%f\n",&x,&y,&u,&v) == 4) {
        index++;
        if(u < 0.5) num[0]++;
        if(u < 0.6) num[1]++;
        if(u < 0.7) num[2]++;
        if(u < 0.8) num[3]++;
        if(u < 0.9) num[4]++;
        if(u < 1.0) num[5]++;
        if(u < 1.1) num[6]++;
    }
    fclose(fp);
   
    /* write output*/
    fpw = safe_fopen(OUTPUT3,"w");
    fprintf(fpw, "threshold,points,percentage\n");
    for(i=0;i<NUMOFTHRESHOLD;i++){
        fprintf(fpw, "%.6f,%d,%.6f\n",start+i*0.1,num[i],(float)num[i]*100/index);
    }
    
    fclose(fpw);
}

void wakevis(const char* flow_file)
{
    int i,j;
    int n = 12; // Location in x for wake visualization
    float* yheight;
    float x,y,u,v;
    struct FlowPoint points[n];
    char buf[MAX_BUF_LEN];
    FILE *fp,*fpw;

    /* initialize u */
    for (i = 0; i < n; i++){
        points[i].u = FLT_MIN;
    }

    yheight = (float*) calloc(n,sizeof(float));
    
    /* read data */
    fp = safe_fopen(flow_file, "r");
    fgets(buf, MAX_BUF_LEN,fp); //skip first line
    while(fscanf(fp,"%f,%f,%f,%f\n",&x,&y,&u,&v) == 4) {
        int index = ((x+THRESHOLD)/STEP);
        if(x < index*STEP + THRESHOLD && x > index*STEP - THRESHOLD){
            if(u > points[index-2].u){
                points[index-2].u = u;
                points[index-2].x = x;
                points[index-2].y = y;
            }
            else if(u == points[index-2].u && y < points[index-2].y){
                points[index-2].u = u;
                points[index-2].x = x;
                points[index-2].y = y;
            }
        }
    }
    fclose(fp);

    /* write output */
    fpw = safe_fopen(OUTPUT4, "w");
    fprintf(fpw,"x,y_h\n");
    for(i=0; i<n; i++){
        fprintf(fpw,"%.6f,%.6f\n",points[i].x,points[i].y);
        yheight[i] = ceil(fabs(points[i].y)*10);
    }
    fclose(fpw);
    /* Task 4: Part 2, nothing is to be changed here
       Remember to output the spacing into the array yheight
       for this to work. You also need to initialize i,j and 
       yheight so the skeleton as it stands will not compile */
     
    FILE *ft42;
    ft42 = fopen("task4_2.txt","w");
    for (j = 11; j>=0; j--){
	for (i=0;i<yheight[j]-yheight[0]+4;i++){
 	    fprintf(ft42, " ");
	}
    	fprintf(ft42, "*\n");
    }
    for (i=0;i<5; i++){
    	fprintf(ft42, "III\n");
    }
    for(j = 0; j<12; j++ ){
    	for (i=0;i<yheight[j]-yheight[0]+4;i++){
    	    fprintf(ft42, " ");
    	}
    	fprintf(ft42, "*\n");
    }
    fclose(ft42);
    
    /* Cleanup */
    free(yheight);
}
