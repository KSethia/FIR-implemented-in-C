/*************************************************************************************
			       DEPARTMENT OF ELECTRICAL AND ELECTRONIC ENGINEERING
					   		     IMPERIAL COLLEGE LONDON 

 				      EE 3.19: Real Time Digital Signal Processing
					       Dr Paul Mitcheson and Daniel Harvey

				        		  LAB 3: Interrupt I/O

 				            ********* I N T I O. C **********

  Demonstrates inputing and outputing data from the DSK's audio port using interrupts. 

 *************************************************************************************
 				Updated for use on 6713 DSK by Danny Harvey: May-Aug 2006
				Updated for CCS V4 Sept 10
 ************************************************************************************/
/*
 *	You should modify the code so that interrupts are used to service the 
 *  audio port.
 */
/**************************** Pre-processor statements ******************************/

#include <stdlib.h>
//  Included so program can make use of DSP/BIOS configuration tool.  
#include "dsp_bios_cfg.h"

/* The file dsk6713.h must be included in every program that uses the BSL.  This 
   example also includes dsk6713_aic23.h because it uses the 
   AIC23 codec module (audio interface). */
#include "dsk6713.h"
#include "dsk6713_aic23.h"

// math library (trig functions)


// Some functions to help with writing/reading the audio ports when using interrupts.
#include <helper_functions_ISR.h>

#include "fir_coeff.txt"

#define N 429

float x[N] = {0};
float x2[N*2] = {0};
float conv_out = 0;

int i=0;
int index1=0;
int index2=0;
int ptr = N-1;

/******************************* Global declarations ********************************/

/* Audio port configuration settings: these values set registers in the AIC23 audio 
   interface to configure it. See TI doc SLWS106D 3-3 to 3-10 for more info. */
DSK6713_AIC23_Config Config = { \
			 /**********************************************************************/
			 /*   REGISTER	            FUNCTION			      SETTINGS         */ 
			 /**********************************************************************/\
    0x0017,  /* 0 LEFTINVOL  Left line input channel volume  0dB                   */\
    0x0017,  /* 1 RIGHTINVOL Right line input channel volume 0dB                   */\
    0x01f9,  /* 2 LEFTHPVOL  Left channel headphone volume   0dB                   */\
    0x01f9,  /* 3 RIGHTHPVOL Right channel headphone volume  0dB                   */\
    0x0011,  /* 4 ANAPATH    Analog audio path control       DAC on, Mic boost 20dB*/\
    0x0000,  /* 5 DIGPATH    Digital audio path control      All Filters off       */\
    0x0000,  /* 6 DPOWERDOWN Power down control              All Hardware on       */\
    0x0043,  /* 7 DIGIF      Digital audio interface format  16 bit                */\
    0x008d,  /* 8 SAMPLERATE Sample rate control             8 KHZ                 */\
    0x0001   /* 9 DIGACT     Digital interface activation    On                    */\
			 /**********************************************************************/
};


// Codec handle:- a variable used to identify audio interface  
DSK6713_AIC23_CodecHandle H_Codec;

 /******************************* Function prototypes ********************************/
void init_hardware(void);     
void init_HWI(void);
void ISR_AIC1(void); 
float non_circ_FIR(float samp);    
float circ_FIR(float samp);
float sym_circ_FIR(float samp);  
float double_sym_circ_FIR(float samp);  
/********************************** Main routine ************************************/
void main(){      

 
	// initialize board and the audio port
  init_hardware();
	
  /* initialize hardware interrupts */
  init_HWI();
  	 		
  /* loop indefinitely, waiting for interrupts */  					
  while(1) 
  {};
  
}
        
/********************************** init_hardware() **********************************/  
void init_hardware()
{
    // Initialize the board support library, must be called first 
    DSK6713_init();
    
    // Start the AIC23 codec using the settings defined above in config 
    H_Codec = DSK6713_AIC23_openCodec(0, &Config);

	/* Function below sets the number of bits in word used by MSBSP (serial port) for 
	receives from AIC23 (audio port). We are using a 32 bit packet containing two 
	16 bit numbers hence 32BIT is set for  receive */
	MCBSP_FSETS(RCR1, RWDLEN1, 32BIT);	

	/* Configures interrupt to activate on each consecutive available 32 bits 
	from Audio port hence an interrupt is generated for each L & R sample pair */	
	MCBSP_FSETS(SPCR1, RINTM, FRM);

	/* These commands do the same thing as above but applied to data transfers to  
	the audio port */
	MCBSP_FSETS(XCR1, XWDLEN1, 32BIT);	
	MCBSP_FSETS(SPCR1, XINTM, FRM);	
	

}

/********************************** init_HWI() **************************************/  
void init_HWI(void)
{
	IRQ_globalDisable();			// Globally disables interrupts
	IRQ_nmiEnable();				// Enables the NMI interrupt (used by the debugger)
	IRQ_map(IRQ_EVT_RINT1,4);		// Maps an event to a physical interrupt -> ISR_AIC1
	IRQ_enable(IRQ_EVT_RINT1);		// Enables the event	
	IRQ_globalEnable();				// Globally enables interrupts
} 

/******************** ISR_AIC1() ***********************/  
void ISR_AIC1(void)
{	/* double sized buffer, symmetrically using coeffiecients in b, and circular buffer in x2
	 * using inlining to improve speed 
	*/
	float y=0;
	int i=0;
	// new sample overwrites the oldest sample in buffer x2
	x2[ptr] = mono_read_16Bit();
	
	// double sized buffer repeats the overwrite at +N of the ptr
	x2[ptr+N] = x2[ptr];

	index1 = ptr;
	index2 = ptr+N-1;
	/* a for loop cycling from zero to half way through the total length of samples
	* equivalent to 1/4 of the entire length of the buffer x2
	*/
	for(i=0; i<(N-1)/2; i++)
	{
		/* convolution of x2 with b, since b is symmetric can use the same value of b for
		 * the current x2[] and the mirrored position in x2[] around the centre
		 * */
		y += (x2[index1+i]+x2[index2-i])*b[i];
	}
	
	/* As the size of the buffer is odd, we do not want the centre value to be added twice
	 * */ 
	y += x2[index1+(N-1)/2]*b[(N-1)/2];
	
	//decrement the pointer to move throught the array
	ptr--;
	
	//check and fix for wrap around
	if(ptr == -1)
	{
		ptr = N-1;
	}
	
	mono_write_16Bit((short)y);	
}


//void ISR_AIC1(void){
//	//Read in a sample from the codec and store it in samp.
//	float samp = mono_read_16Bit();	
//	//Filtering function is called, returning correct value, converted to short.
//	float conv_out = non_circ_FIR(samp);
//	//Writes filtered value
//	mono_write_16Bit((short)conv_out);	
//
//}

float non_circ_FIR(float samp){
	int i;
	float y=0;
	
	/* shifts all the elements one position down in the array
	 * with the oldest dropping off, no longer needed
	*/
	for (i = N-1; i>0; i--)
	{
		x[i] = x[i-1];
	}
	// puts the new sample into the top position
	x[0] = samp;
	
	/*perfroms the convolution between x and b 
	 * cycling through array from 0 to the last array position
	 */
	for (i = 0; i<N; i++)
	{
		// multiply accumulate process
		y += x[i]*b[i];
	}
	
	//returns the correct value for y to the ISR
	return (y);
}
 
float circ_FIR(float samp){
	int i=0, index=0;
	float y=0;
	/*overwrites the oldest sample with the newest
	 */ 
	x[ptr] = samp;
	index = ptr;
	
	/*cycles through the buffer from 0 to full length
	 */
	for(i=0; i<N; i++)
	{
		/*checks for reaching the end of the array
		 */
		if(index+i == N)
		{
			//stores the value of i
			int count = i;
			int k = 0;
			/*wraps around and completes the convolution,
			 stopping at the correct position
			*/
			for(k=0; k<index; k++)
			{
				//multiply accumulate
				y += x[k]*b[count+k];
			}
			// causes exit from for loop asconvolution is complete
			i = N;
		}
		else{
		y += x[index+i]*b[i];
		}
	}
	//moves the pointer through the array for new sample
	ptr--;
	
	//wrap around
	if(ptr == -1){
		ptr = N-1;
	}
	
	//returns the correct value for y to the ISR
	return y;
}

float sym_circ_FIR(float samp){												
	// use two pointers to select the values to be multiplied by the b value
	int i=0, index1=0, index2=0;
	float y=0;
	//new sample overwrites the oldest sample in buffer x
	x[ptr] = samp;
	index1 = ptr;
	index2 = ptr-1;
	/*if pointer 1 is at position 0, the corresponding starting position for 
	 *the second pointer is the last value in the array*/
	if(index1==0)
	{
		index2 = N-1;
	}
	/*a for loop cycling from zero to half way through the total length of samples, since we can 
	 * multiply two values of x by the value of b in each loop
	 */
	for(i=0; i<(N-1)/2; i++)
	{
		/*multiply accumulate of x with b 
		 */
		y += x[index1]*b[i] + x[index2]*b[i];
		//increments/decrements index values
		index1++;
		index2--;
		// checks to see if the index's have reached the end of the buffer, and wraps around
		if(index1 == N)
		{
			index1 = 0;
		}
		if(index2 == -1)
		{
			index2 = N-1;
		}
	}
	if(ptr <= (N-1)/2)
	{
		y+=x[ptr+(N-1)/2]*b[(N-1)/2];
	}
	else
	{
		y+=x[ptr-(N-1)/2]*b[(N-1)/2];
	}
	//moves the pointer through the array for new sample
	ptr--;
	//wrap around
	if(ptr == -1){
		ptr = N-1;
	}
	//returns the correct value for y to the ISR
	return y;
}

float double_sym_circ_FIR(float samp){
	// double sized buffer, symmetrically using coeffiecients in b, and circular buffer in x2 
	float y=0;
	int i=0, index1=0, index2=0;
	
	// new sample overwrites the oldest sample in buffer x2
	x2[ptr] = samp;
	// double sized buffer repeats the overwrite at +N of the ptr
	x2[ptr+N] = x2[ptr];

	index1 = ptr;
	index2 = ptr+N-1;
	/* a for loop cycling from zero to half way through the total length of samples
	* equivalent to 1/4 of the entire length of the buffer x2
	*/
	for(i=0; i<(N-1)/2; i++)
	{
		y += (x2[index1+i]+x2[index2-i])*b[i];
	}
	/* Because the size of the buffer is odd, we do not want the centre value to be added twice
	 * */ 
	y += x2[index1+(N-1)/2]*b[(N-1)/2];
	
	
	//decrement the pointer to move throught the array
	ptr--;
	
	//check and fix for wrap around
	if(ptr == -1)
	{
		ptr = N-1;
	}
	
	//return the final value of y 
	return y;
}
