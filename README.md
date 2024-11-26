# System Programming Lab 11 Multiprocessing

## Introduction
 This mandelbrot plot is scaled down over 50 images and these images can be compiled with the ffmpeg using the command line:

    ffmpeg -i mandel%d.jpg mandel.mpg


## Results
![Graph of results](Picture1.png)

The graph represents the asymptotic behavior. The computer only has so many logical cores available to it, so as the number of processes is increased, the performance should hypothetically increase infinitely, however, the limitation of the cores results in the asymptotic behavior. 

It is important to note, that the first frames of the program take much more time than the later ones. Thus, the process who receives the first ones will take longer and the other processes have to wait. 



