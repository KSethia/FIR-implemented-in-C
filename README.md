# FIR-implemented-in-C
Finite Impulse Response filter to be used with a Texas Instrument C6713 processor and AIC23 audio coedc

/*-------------------------------------------------*/

As part of my Real-Time Digital Signal Processing course, a Finite Impulse Response filter had to be made and implemented.

The hardware initialisations at the top of the initio.c file was written by Dr Paul Mitcheson and Daniel Harvey, however all the processing code was written by myself and my lab partner.

This skeleton code allows us to implement the FIR onto the DSP board and test it with an oscilloscope or frequency spectrum analyser. 

Included in the repository is the corresponding report which goes into detail about how the FIR was generated in Matlab and implemented in C. This program was optimised for speed (clock cycles), and a description how this was done can be found in the report.

Whilst this code won’t work or compile without the configuration file, it provides a good example of the use of buffers and methods for optimisation. 

/*-------------------------------------------------*/
