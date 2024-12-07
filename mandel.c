/**
 * @file mandel.c
 * @brief Main program file for Multiprocessing lab 2024
 * 
 * Course: CPE2600
 * Section: 011
 * Assignment: Multiprocessing
 * Name: Matthew Senn
 * 
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "jpegrw.h"
#include <math.h>
#include <pthread.h>

struct threadargs_t {
	double xmin,xmax,ymin,ymax;
	int max,start_width,end_width;
	imgRawImage* img;

};

// local routines
static int iteration_to_color( int i, int max );
static int iterations_at_point( double x, double y, int max );
static void compute_image( imgRawImage *img, double xmin, double xmax,
									double ymin, double ymax, int max );
static void show_help();
void compute_slice(imgRawImage* img, double xmin, double xmax, double ymin, double ymax, int max, int start_width, int stop_width);
void* handle_thread(void* arg);

int main( int argc, char *argv[] )
{
	char c;

	// These are the default configuration values used
	// if no command line arguments are given.
	double xcenter = 0;
	double ycenter = 0;
	double xscale = 4;
	double yscale = 0; // calc later
	int    image_width = 1000;
	int    image_height = 1000;
	int    max = 1000;
	int    nprocs = 1;
	int    total_images = 50;
	int    nthreads = 1;

	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:o:h:n:T:R"))!=-1) {
		switch(c) 
		{
			case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				xscale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				break;
			case 'H':
				image_height = atoi(optarg);
				break;
			case 'm':
				max = atoi(optarg);
				break;
			case 'o':
				// outfile = optarg;
				printf(" ");
				break;
			case 'h':
				show_help();
				exit(1);
				break;
			case 'n':
				nprocs = atoi(optarg);
				break;
			case 'T':
				nthreads = atoi(optarg);
				printf("Number of threads %d\n",nthreads);
				if(nthreads > 20) {
					printf("Too many thread requested!");
					nthreads = 20;
				} else if (nthreads < 1){
					nthreads = 1;
				}
				break; 
		}
	}
	yscale = xscale / image_width * image_height;
	
	int frames_per_process = total_images / nprocs;
    int remaining_frames = total_images % nprocs;

    pid_t pids[nprocs];
    for (int i = 0; i < nprocs; i++) {
		//Process****************************************************************
        if ((pids[i] = fork()) == 0) { // Child process

			double curX = xscale;
			double curY = yscale;
            int start_frame = i * frames_per_process;
            int end_frame = start_frame + frames_per_process;
            if (i == nprocs - 1) {
                end_frame += remaining_frames; // Last process handles the extra frames
            }
			
			//generating each image from start to end 
            for (int frame = start_frame; frame < end_frame; frame++) {
				
				imgRawImage* img = initRawImage(image_width,image_height);
				// Fill it with a black
				setImageCOLOR(img,0);

				//thread creation
				pthread_t threads[nthreads];
				int img_width = img->width;
				curX = xscale - frame;
				curY = curX / image_width*image_height;
				char outfile[256];
            	snprintf(outfile, sizeof(outfile), "mandel%d.jpg", frame);

				for(int i = 0; i < nthreads; i++){
					int start_width = img_width/nthreads*i;
					int end_width = start_width + img_width/nthreads;
					if(i == nthreads-1) end_width = img_width;
					//void compute_slice(imgRawImage* img, double xmin, double xmax, double ymin, double ymax, int max, int start_width, int stop_width) {
					//img,xcenter-xscale/2,xcenter+xscale/2,ycenter-yscale/2,ycenter+yscale/2,max
					struct threadargs_t args;
					args.img = img;
					args.xmin = xcenter-curX/2;
					args.xmax = xcenter+curX/2;
					args.ymin = ycenter-yscale/2;
					args.max = max;
					args.start_width = start_width;
					args.end_width = end_width;

					pthread_create(&threads[i], NULL, &handle_thread, (void*)&args);
					pthread_join(threads[i], NULL);
				}
				// generate_frame(image_width, image_height, xcenter, ycenter, curX, curY, outfile, max);
				printf("Generating image %d: xscale=%lf yscale=%lf outfile=%s\n",frame, curX, curY, outfile);

				// Save the image in the stated file.
				storeJpegImageFile(img,outfile);

				// free the mallocs
				freeRawImage(img);
            }
            exit(0);
        } else if (pids[i] < 0) { // Fork failed
            perror("fork");
            exit(-1);
        }
    }

    // Parent  waits for all children to finish
    for (int proc = 0; proc < nprocs; proc++) {
        wait(NULL);
    }

    printf("All images generated successfully.\n");
    return 0;
}

void* handle_thread(void* arg){
	struct threadargs_t* args = (struct threadargs_t*) arg;
	//void compute_slice(imgRawImage* img, double xmin, double xmax, double ymin, double ymax, int max, int start_width, int stop_width) {
	compute_slice(args->img,args->xmin,args->xmax,args->ymin,args->ymax,args->max,args->start_width,args->end_width);
	return (void*) 0;
}


/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iter;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/

void compute_image(imgRawImage* img, double xmin, double xmax, double ymin, double ymax, int max )
{
	int i,j;

	int width = img->width;
	int height = img->height;

	// For every pixel in the image...

	for(j=0;j<height;j++) {

		for(i=0;i<width;i++) {

			// Determine the point in x,y space for that pixel.
			double x = xmin + i*(xmax-xmin)/width;
			double y = ymin + j*(ymax-ymin)/height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,max);

			// Set the pixel in the bitmap.
			setPixelCOLOR(img,i,j,iteration_to_color(iters,max));
		}
	}
}

void compute_slice(imgRawImage* img, double xmin, double xmax, double ymin, double ymax, int max, int start_width, int stop_width) {
	int i,j;

	int height = img->height;
	int width = img->width;

	// For every pixel in the image...

	for(j=0;j<height;j++) {

		for(i=start_width;i<stop_width;i++) {

			// Determine the point in x,y space for that pixel.
			double x = xmin + i*(xmax-xmin)/width;
			double y = ymin + j*(ymax-ymin)/height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,max);

			// Set the pixel in the bitmap.
			setPixelCOLOR(img,i,j,iteration_to_color(iters,max));
		}
	}
}


/*
Convert a iteration number to a color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/
int iteration_to_color( int iters, int max )
{
	int color = 0xF0F0FF*iters/(double)max;
	return color;
}


// Show help message
void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates (X-axis). (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=1000)\n");
	printf("-H <pixels> Height of the image in pixels. (default=1000)\n");
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}
