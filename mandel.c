/// 
//  mandel.c
//  Based on example code found here:
//  https://users.cs.fiu.edu/~cpoellab/teaching/cop4610_fall22/project3.html
//
//  Converted to use jpg instead of BMP and other minor changes
//  
///
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include "jpegrw.h"

// local routines
static int iteration_to_color( int i, int max );
static int iterations_at_point( double x, double y, int max );
static void compute_image( imgRawImage *img, double xmin, double xmax,
									double ymin, double ymax, int max );
static void show_help();
static void generate_frame(int width, int height, int xcenter, int ycenter, int xscale, int yscale, char* outfile, int max);


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

	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:o:h:n:p"))!=-1) {
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
				printf("%s",optarg);
				nprocs = atoi(optarg);
				break;
		}
	}
	printf("nprocs%d\n",nprocs);

	// Calculate y scale based on x scale (settable) and image sizes in X and Y (settable)
	// yscale = xscale / image_width * image_height;

	// Display the configuration of the image.
	// printf("mandel: x=%lf y=%lf xscale=%lf yscale=%1f max=%d outfile=%s\n",xcenter,ycenter,xscale,yscale,max,outfile);

	// for (int img = 0; img < total_images; img++) {
    //     if (img % nprocs == 0 && img > 0) {
    //         // Parent process waits for all child processes to finish before starting next batch
    //         for (int p = 0; p < nprocs; p++) {
    //             wait(NULL);
    //         }
    //     }

    //     pid_t pid = fork();
    //     if (pid == 0) { // Child process
	// 		// xscale = 4 - img; // Example: Decrement `xscale` with each image
	// 		// yscale = xscale / image_width * image_height; // Recalculate `yscale` proportionally

	// 		xcenter = -0.5 + 0.1 * img; // Move the center
    // 		ycenter = -0.5 + 0.05 * img; // Adjust center vertically

    //         // double xscale = initial_xscale - img * xscale_decrement; // Vary xscale
    //         // double yscale = xscale / image_width * image_height; // Adjust yscale proportionally
    //         char outfile[256];
    //         snprintf(outfile, sizeof(outfile), "madel%d.jpg", img);

    //         printf("Generating image %d: xscale=%lf yscale=%lf outfile=%s\n",
    //                img, xscale, yscale, outfile);

    //         generate_frame(image_width, image_height, xcenter, ycenter, xscale, yscale, outfile, max);
    //         exit(0); // Ensure child process exits after generating the image
    //     }
    // }

    // // Wait for the final batch of processes
    // for (int img = total_images - nprocs; img < total_images; img++) {
    //     wait(NULL);
    // }

    // printf("All images generated successfully.\n");



	yscale = xscale * (double)image_height / image_width;

	int frames_per_proc = total_images / nprocs; // Frames assigned to each process
    int remaining_frames = total_images % nprocs; // Extra frames if not divisible evenly

    for (int proc = 0; proc < nprocs; proc++) {
        pid_t pid = fork();

        if (pid == 0) { // Child process
            int start_frame = proc * frames_per_proc;
            int end_frame = start_frame + frames_per_proc;

            // Assign remaining frames to the last process
            if (proc == nprocs - 1) {
                end_frame += remaining_frames;
            }

            printf("Process %d: Handling frames %d to %d\n", proc, start_frame, end_frame - 1);

            for (int img = start_frame; img < end_frame; img++) {
                int current_xscale = xscale - img; // Example: Decrease xscale per frame
                int current_yscale = current_xscale / image_width * image_height; // Recalculate yscale

                char outfile[256];
                snprintf(outfile, sizeof(outfile), "madel%d.jpg", img);

                printf("Process %d generating image %d: xscale=%d yscale=%d outfile=%s\n",
                       proc, img, current_xscale, current_yscale, outfile);

                generate_frame(image_width, image_height, xcenter, ycenter, current_xscale, current_yscale, outfile, max);
            }

            exit(0); // Ensure child process exits after finishing its frames
        }
    }

    // Parent process waits for all children to finish
    for (int proc = 0; proc < nprocs; proc++) {
        wait(NULL);
    }

    printf("All images generated successfully.\n");
    return 0;
}

// Function to generate a Mandelbrot image
void generate_frame(int width, int height, int xcenter, int ycenter, int xscale, int yscale, char* outfile, int max){
	imgRawImage* img = initRawImage(width,height);

	// Fill it with a black
	setImageCOLOR(img,0);

	// Compute the Mandelbrot image
	compute_image(img,xcenter-xscale/2,xcenter+xscale/2,ycenter-yscale/2,ycenter+yscale/2,max);

	// Save the image in the stated file.
	storeJpegImageFile(img,outfile);

	// free the mallocs
	freeRawImage(img);

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
