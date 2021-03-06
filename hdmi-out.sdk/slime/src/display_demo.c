/************************************************************************/
/*																		*/
/*	display_demo.c	--	ZYBO Display demonstration 						*/
/*																		*/
/************************************************************************/
/*	Author: Sam Bobrowicz												*/
/*	Copyright 2016, Digilent Inc.										*/
/************************************************************************/
/*  Module Description: 												*/
/*																		*/
/*		This file contains code for running a demonstration of the		*/
/*		HDMI output capabilities on the ZYBO. It is a good	            */
/*		example of how to properly use the display_ctrl drivers.	    */
/*																		*/
/************************************************************************/
/*  Revision History:													*/
/* 																		*/
/*		2/5/2016(SamB): Created											*/
/*																		*/
/************************************************************************/


//TODO Doubelt buffer	:	Controller

/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

#include "display_demo.h"
#include "display_ctrl/display_ctrl.h"
#include <stdio.h>
#include "xuartps.h"
#include "math.h"
#include <ctype.h>
#include <stdlib.h>
#include "xil_types.h"
#include "xil_cache.h"
#include "timer_ps/timer_ps.h"
#include "xparameters.h"
#include "xgpio.h"

/*
 * XPAR redefines
 */
#define DYNCLK_BASEADDR XPAR_AXI_DYNCLK_0_BASEADDR
#define VGA_VDMA_ID XPAR_AXIVDMA_0_DEVICE_ID
#define DISP_VTC_ID XPAR_VTC_0_DEVICE_ID
#define VID_VTC_IRPT_ID XPS_FPGA3_INT_ID
#define VID_GPIO_IRPT_ID XPS_FPGA4_INT_ID
#define SCU_TIMER_ID XPAR_SCUTIMER_DEVICE_ID
#define UART_BASEADDR XPAR_PS7_UART_1_BASEADDR
#define BTNS_DEVICE_ID XPAR_AXI_GPIO_BTN_DEVICE_ID
#define CONTROLLER_1_DEVICE_ID XPAR_AXI_GPIO_CONTROLLER_1_DEVICE_ID
#define CONTROLLER_2_DEVICE_ID XPAR_AXI_GPIO_CONTROLLER_2_DEVICE_ID

/* ------------------------------------------------------------ */
/*				Global Variables								*/
/* ------------------------------------------------------------ */

int backgroundRed = 0;
int backgroundBlue = 255;
int backgroundGreen = 0;

u8 figureColorRed = 255;
u8 figureColorBlue = 0;
u8 figureColorGreen = 0;

//Player Colors RGB
u8 playerOneRedColor = 255;
u8 playerOneBlueColor = 255;
u8 playerOneGreenColor = 0;

u8 playerTwoRedColor = 255;
u8 playerTwoBlueColor = 0;
u8 playerTwoGreenColor = 255;


u32 startPixelAddr;

double movementSpeed = 5.0;	//Player speed

int jumpHeight = 10;	//How hight to jump straight
int jumpSpeed = 1000; // Jump delay in micro seconds. - Lower number = high speed	-	High number = low speed

int sizeX = 60*2;	//Size of square on the x axis
int sizeY = 30*2; //Size of square on the x axis

int fps = 60;	//Frames pr. second.

int btn_value;	//Button input value
int btn_delay = 20000;	//Controls the speed at witch the button on the ZYBO is delayed between each move

int nextFrame = 0;
int lastFrame;

//Controller input value
int controllerOneValue;
int controllerTwoValue;


/* ------------------------------------------------------------ */
/*				Driver Structs   								*/
/* ------------------------------------------------------------ */

DisplayCtrl dispCtrl;
XAxiVdma vdma;	//Video Direct memory access struct
XGpio BTNinst; // Button struct;
XGpio ControllerOneInstance, ControllerTwoInstance;

/*
 * Framebuffers for video data
 */
u8  frameBuf[DISPLAY_NUM_FRAMES][DEMO_MAX_FRAME] __attribute__((aligned(0x20)));
u8 *pFrames[DISPLAY_NUM_FRAMES]; //array of pointers to the frame buffers


/* ------------------------------------------------------------ */
/*				Prototypes							*/
/* ------------------------------------------------------------ */

 struct Players{

	u32 newPosition;
	u32 oldPosition;
	u8 blue;
	u8 green;
	u8 red;
	char left;
	char right;
	char jump;
	u32 boarderPointer;

}playerOne, playerTwo;



//New func

void moveSquare(u8 *frame, u32 width, u32 height, u32 stride);
void initializeDrivers();

//Player one movement
void movePlayerOneUp(u8 *srcDrame, u8 *nextFrame, u32 stride, int pixelToMove);
void movePlayerOneDown(u8 *srcDrame, u8 *nextFrame, u32 stride, int pixelToMove);
void movePlayerOneLeft(u8 *srcFrame, u8 *nextFrame ,u32 stride, int pixelToMove);
void movePlayerOneRight(u8 *srcFrame, u8 *nextFrame ,u32 stride, int pixelToMove);
void jumpToRightSidePlayerOne(u8 *frame, u32 stride, int jumpHeight);

//Player Two movement
void movePlayerTwoUp(u8 *srcDrame, u8 *nextFrame, u32 stride, int pixelToMove);
void movePlayerTwoDown(u8 *srcDrame, u8 *nextFrame, u32 stride, int pixelToMove);
void movePlayerTwoLeft(u8 *srcFrame, u8 *nextFrame ,u32 stride, int pixelToMove);
void movePlayerTwoRight(u8 *srcFrame, u8 *nextFrame ,u32 stride, int pixelToMove);
void jumpToRightSidePlayerTwo(u8 *frame, u32 stride, int jumpHeight);

/*
void movePlayerTwoUp(u8 *frame, u32 stride, int pixelToMove);
void movePlayerTwoDown(u8 *frame, u32 stride, int pixelToMove);
void movePlayerTwoLeft(u8 *frame, u32 stride, int pixelToMove);
void movePlayerTwoRight(u8 *frame, u32 stride, int pixelToMove);
void advanceJumpPlayerOne(u8 *frame, u32 stride, int jumpHeight);
*/

void makeBackground(u8 *frame, u32 width, u32 height, u32 stride, int red, int blue, int green);
u32 coordinatSystem(int x, int y, u32 stride);
void jumpPlayerOne(u8 *frame, u32 stride, int jumpHeight);
void graphicArray(u32 startPixelAddr, u8 *frame, u32 width, u32 height, u32 stride);
void jumpPlayerTwo(u8 *frame, u32 stride, int jumpHeight);
void initPlayerOne(u32 position, u8 *frame, u8 playerOneGreenColor, u8 playerOneBlueColor, u8 playerOneRedColor, char leftButton, char rightButton, char jumpButton, u32 stride);
void initPlayerTwo(u32 position, u8 *frame, u8 playerTwoGreenColor, u8 playerTwoBlueColor, u8 playerTwoRedColor, char leftButton, char rightButton, char jumpButton, u32 stride);

void drawPlayers(u8 *frame, u32 stride);
int getNextFrame();
int getCurrentFrame();
void drawNextFrame(u8 *srcFrame, u8 *nextFrame ,u32 stride, u32 width, u32 height);
void clearOldFrame(u8 *srcFrame, u8 *nextFrame ,u32 stride, u32 width, u32 height);



int main(void)
{

	initializeDrivers();

	makeBackground(pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE, backgroundRed, backgroundBlue, backgroundGreen);

	moveSquare(pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE);

	return 0;
}


void moveSquare(u8 *frame, u32 width, u32 height, u32 stride){

	u32 startPixelAddr = stride*(height) - (sizeY*stride) + width;
	u32 startPixelAddr2 = stride*(height) - (sizeY*stride) + width*3;

	//startPixelAddr = initPlayer(playerOne, startPixelAddr ,pFrames[dispCtrl.curFrame], 255, 0, 255, 'a', 'd', 'w', DEMO_STRIDE);
	initPlayerOne(startPixelAddr,pFrames[dispCtrl.curFrame], 0, 255, 255, 'a', 'd', 'w', DEMO_STRIDE);
	initPlayerTwo(startPixelAddr2 ,pFrames[dispCtrl.curFrame], 255, 0, 255, '1', '3', '5', DEMO_STRIDE);

	drawPlayers(pFrames[dispCtrl.curFrame], DEMO_STRIDE);

	while(1)
	{


		/*
		while (!XUartPs_IsReceiveData(UART_BASEADDR) && btn_value == 0x0)
		{
			btn_value = XGpio_DiscreteRead(&BTNinst, 1);
		}
		*/

		controllerOneValue =  XGpio_DiscreteRead(&ControllerOneInstance, 1);
		controllerTwoValue =  XGpio_DiscreteRead(&ControllerTwoInstance, 1);


		xil_printf("%d   \r\n   ", controllerTwoValue);


		if (controllerOneValue == 13 && controllerTwoValue == 13)
		{
			movePlayerOneRight(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, movementSpeed);
			movePlayerTwoRight(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, movementSpeed);

/*
			nextFrame = dispCtrl.curFrame + 1;
			lastFrame = dispCtrl.curFrame;
			if (nextFrame >= DISPLAY_NUM_FRAMES)
			{
				nextFrame = 0;
			}
*/
			drawNextFrame(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
			//TimerDelay(1000000/fps);
			DisplayChangeFrame(&dispCtrl, nextFrame);
			clearOldFrame(dispCtrl.framePtr[lastFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
			xil_printf("%dTEST   \r\n   ");
			continue;
		}



		switch(controllerOneValue)
		{
		case 7 :	// binary 0x0x
			movePlayerOneLeft(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, movementSpeed);
			break;

		case 13:
			movePlayerOneRight(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, movementSpeed);

			break;

		case 11:
			jumpPlayerOne(pFrames[dispCtrl.curFrame], DEMO_STRIDE, jumpHeight);

			break;
		}

		switch(controllerTwoValue)
		{
		case 7 :	// binary 0x0111
			movePlayerTwoLeft(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, movementSpeed);
			break;

		case 13:	// binary 0x1101
			movePlayerTwoRight(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, movementSpeed);

			break;

		case 11:	// binary 0x1110
			jumpPlayerTwo(pFrames[dispCtrl.curFrame], DEMO_STRIDE, jumpHeight);

			break;
		}



/*
		userInput = XUartPs_ReadReg(UART_BASEADDR, XUARTPS_FIFO_OFFSET);

		//Player one cases:
		switch(userInput)
		{

			case 'a' :	//Move player one left
				movePlayerOneLeft(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, movementSpeed);
				break;

			case 'd' :	// Move player one righ
				movePlayerOneRight(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, movementSpeed);
				break;

			case 'w' :
				jumpPlayerOne(pFrames[dispCtrl.curFrame], DEMO_STRIDE, jumpHeight);
				break;

			case 'e' :
				jumpToRightSidePlayerOne(pFrames[dispCtrl.curFrame], DEMO_STRIDE, jumpHeight);
				break;

			case 'h' :
				graphicArray(startPixelAddr, pFrames[dispCtrl.curFrame], dispCtrl.vMode.width, dispCtrl.vMode.height, DEMO_STRIDE);
				break;
		}

		//Player two cases:
		switch(btn_value)
		{
			case 0x1 :
				movePlayerTwoLeft(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, movementSpeed);
				break;

			case 0x2 :
				movePlayerTwoRight(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, movementSpeed);
				break;

			case 0x4 :
				jumpPlayerTwo(pFrames[dispCtrl.curFrame], DEMO_STRIDE, jumpHeight);
				TimerDelay(btn_delay);
				break;
		}
			btn_value = 0;

*/
	}
}


void makeBackground(u8 *frame, u32 width, u32 height, u32 stride, int red, int blue, int green){

	for(int i = 0 ; i < (width)*4*height*3  ; i+=4)
	{
		frame[i] = blue;
		frame[i + 1] = green;
		frame[i + 2] = red;
	}
	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
}

u32 coordinatSystem(int x, int y, u32 stride){

	return x*4+y*stride;
}


void setResulution(char userInput){

	switch(userInput){
		case '1':
			//status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_640x480);
			DisplayStart(&dispCtrl);
			//fResSet = 1;
			break;
		case '2':
			//status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_800x600);
			DisplayStart(&dispCtrl);
			//fResSet = 1;
			break;
		case '3':
			//status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1280x720);
			DisplayStart(&dispCtrl);
			//fResSet = 1;
			break;
		case '4':
			//status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1280x1024);
			DisplayStart(&dispCtrl);
			//fResSet = 1;
			break;
		case '5':
			//status = DisplayStop(&dispCtrl);
			DisplaySetMode(&dispCtrl, &VMODE_1920x1080);
			DisplayStart(&dispCtrl);
			//fResSet = 1;
			break;
	}
}


void initializeDrivers()
{

	/*
	 * Initialize an array of pointers to the 3 frame buffers
	 */
	for (int i = 0; i < DISPLAY_NUM_FRAMES; i++)
	{
		pFrames[i] = frameBuf[i];
	}


	int status;

	// Initialize buttons (only used for testing purposes)
    status = XGpio_Initialize(&BTNinst, BTNS_DEVICE_ID);
    if(status != XST_SUCCESS) {xil_printf("Could not instantiate buttons on ZYBO board\r\n"); return;}

    // Initialize controller one
    status = XGpio_Initialize(&ControllerOneInstance, CONTROLLER_1_DEVICE_ID);
    if(status != XST_SUCCESS) {xil_printf("Could not instantiate controller one\r\n"); return;}

    // Initialize controller two
     status = XGpio_Initialize(&ControllerTwoInstance, CONTROLLER_2_DEVICE_ID);
     if(status != XST_SUCCESS) {xil_printf("Could not instantiate controller two\r\n"); return;}

    // Set all buttons direction to inputs
    XGpio_SetDataDirection(&BTNinst, 1, 0xFF);

    // Set all buttons direction to inputs on controller one
    XGpio_SetDataDirection(&ControllerOneInstance, 1, 0xFF);


    // Set all buttons direction to inputs on controller two
    XGpio_SetDataDirection(&ControllerTwoInstance, 1, 0xFF);

	int Status;
	XAxiVdma_Config *vdmaConfig;
	int i;


	 //Initialize an array of pointers to the 3 frame buffers
	for (i = 0; i < DISPLAY_NUM_FRAMES; i++){pFrames[i] = frameBuf[i];}


	 //Initialize a timer used for a simple delay
	TimerInitialize(SCU_TIMER_ID);


	//Initialize VDMA driver
	vdmaConfig = XAxiVdma_LookupConfig(VGA_VDMA_ID);
	if (!vdmaConfig)
	{
		xil_printf("No video DMA found for ID %d\r\n", VGA_VDMA_ID);
		return;
	}
	Status = XAxiVdma_CfgInitialize(&vdma, vdmaConfig, vdmaConfig->BaseAddress);
	if (Status != XST_SUCCESS)
	{
		xil_printf("VDMA Configuration Initialization failed %d\r\n", Status);
		return;
	}


	 //Initialize the Display controller and start it

	Status = DisplayInitialize(&dispCtrl, &vdma, DISP_VTC_ID, DYNCLK_BASEADDR, pFrames, DEMO_STRIDE);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Display Ctrl initialization failed during demo initialization%d\r\n", Status);
		return;
	}
	Status = DisplayStart(&dispCtrl);
	if (Status != XST_SUCCESS)
	{
		xil_printf("Couldn't start display during demo initialization%d\r\n", Status);
		return;
	}

	// Initialize resolution for the monitor
	DisplaySetMode(&dispCtrl, &VMODE_1920x1080);
	DisplayStart(&dispCtrl);

}


void graphicArray(u32 startPixelAddr, u8 *frame, u32 width, u32 height, u32 stride){

	int sp = width/2 + stride*200;

	int array[25] = {
			 0 , 0 , 1 , 0 , 0,
			 0 , 0 , 1 , 0 , 0,
			 1 , 1 , 1 , 1 , 1,
			 0 , 0 , 1 , 0 , 0,
			 0 , 0 , 1 , 0 , 0};

	int counter = 0;
	int j = 0;
	for(int i = 0 ; i < 25 ; i++)
		{
		if(array[i] == 0)
			{
				frame[sp + j] = backgroundBlue;
				frame[sp + j +1] = backgroundGreen;
				frame[sp + j +2] = backgroundRed;
			}
		else if(array[i] == 1)
			{
				frame[sp + j] = figureColorBlue;
				frame[sp + j + 1] = figureColorGreen;
				frame[sp + j + 2] = figureColorRed;
			}
		counter++;
		j += 4;
		if(counter == 5)
			{
				sp += stride-(5*4);
				counter = 0;
			}
		}

	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);

}


void initPlayerOne(u32 position, u8 *frame, u8 green, u8 blue, u8 red, char leftButton, char rightButton, char jumpButton, u32 stride){

	playerOne.newPosition = position;
	playerOne.oldPosition = position;
	playerOne.blue = blue;
	playerOne.red = red;
	playerOne.green = green;
	playerOne.left = leftButton;
	playerOne.right = rightButton;
	playerOne.jump = jumpButton;

}


void initPlayerTwo(u32 position, u8 *frame, u8 green, u8 blue, u8 red, char leftButton, char rightButton, char jumpButton, u32 stride){

	playerTwo.newPosition = position;
	playerTwo.oldPosition = position;
	playerTwo.blue = blue;
	playerTwo.red = red;
	playerTwo.green = green;
	playerTwo.left = leftButton;
	playerTwo.right = rightButton;
	playerTwo.jump = jumpButton;

}


void drawPlayers(u8 *frame, u32 stride){

	for(int i = 0 ; i < sizeY ; i++)
	{
		for(int j = 0 ; j < sizeX*4  ; j+=4)
		{
			//Draw player one
			frame[playerOne.newPosition + j] = playerOne.blue;
			frame[playerOne.newPosition + j + 1] = playerOne.green;
			frame[playerOne.newPosition + j + 2] = playerOne.red;

			//Draw player two
			frame[playerTwo.newPosition + j] = playerTwo.blue;
			frame[playerTwo.newPosition + j + 1] = playerTwo.green;
			frame[playerTwo.newPosition + j + 2] = playerTwo.red;

		}
		playerOne.newPosition += stride;
		playerTwo.newPosition += stride;
	}
	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
	playerOne.newPosition -= (sizeY * stride);
	playerTwo.newPosition -= (sizeY * stride);
}

/* ------------------------------------------------------------ */
/*				Player one movement								*/
/* ------------------------------------------------------------ */


void movePlayerOneRight(u8 *srcDrame, u8 *nextFrames, u32 stride, int pixelToMove){

	playerOne.oldPosition = playerOne.newPosition;
	playerOne.newPosition += 4*pixelToMove;


	nextFrame = dispCtrl.curFrame + 1;
	lastFrame = dispCtrl.curFrame;
	if (nextFrame >= DISPLAY_NUM_FRAMES)
	{
		nextFrame = 0;
	}
	drawNextFrame(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
	//TimerDelay(1000000/fps);
	DisplayChangeFrame(&dispCtrl, nextFrame);
	clearOldFrame(dispCtrl.framePtr[lastFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);


}


void movePlayerOneLeft(u8 *srcDrame, u8 *nextFrames, u32 stride, int pixelToMove){

	playerOne.oldPosition = playerOne.newPosition;
	playerOne.newPosition -= 4*pixelToMove;

	nextFrame = dispCtrl.curFrame + 1;
	lastFrame = dispCtrl.curFrame;
	if (nextFrame >= DISPLAY_NUM_FRAMES)
	{
		nextFrame = 0;
	}
	drawNextFrame(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
	//TimerDelay(1000000/fps);
	DisplayChangeFrame(&dispCtrl, nextFrame);
	clearOldFrame(dispCtrl.framePtr[lastFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);

}

void movePlayerOneUp(u8 *srcDrame, u8 *nextFrame, u32 stride, int pixelToMove){

	playerOne.oldPosition = playerOne.newPosition;
	playerOne.newPosition -= stride*pixelToMove;

}

void movePlayerOneDown(u8 *srcDrame, u8 *nextFrame, u32 stride, int pixelToMove){

	playerOne.oldPosition = playerOne.newPosition;
	playerOne.newPosition += stride*pixelToMove;

}


void jumpPlayerOne(u8 *frame, u32 stride, int jumpHeight){


	for(int i = jumpHeight ; i > 0 ; i--)
	{
		for(int j = 0 ; j < 5 ; j++)
		{
			movePlayerOneUp(pFrames[dispCtrl.curFrame], pFrames[dispCtrl.curFrame], DEMO_STRIDE, i);
			nextFrame = dispCtrl.curFrame + 1;
			lastFrame = dispCtrl.curFrame;
			if (nextFrame >= DISPLAY_NUM_FRAMES)
			{
				nextFrame = 0;
			}
			drawNextFrame(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
			TimerDelay(500000/fps);
			DisplayChangeFrame(&dispCtrl, nextFrame);
			clearOldFrame(dispCtrl.framePtr[lastFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
		}
	}

	for(int i = 1 ; i <= jumpHeight ; i++)
	{
		for(int j = 0 ; j < 5 ; j++)
		{
			movePlayerOneDown(pFrames[dispCtrl.curFrame], pFrames[dispCtrl.curFrame], DEMO_STRIDE, i);
			nextFrame = dispCtrl.curFrame + 1;
			lastFrame = dispCtrl.curFrame;
			if (nextFrame >= DISPLAY_NUM_FRAMES)
			{
				nextFrame = 0;
			}
			drawNextFrame(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
			TimerDelay(500000/fps);
			DisplayChangeFrame(&dispCtrl, nextFrame);
			clearOldFrame(dispCtrl.framePtr[lastFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
		}
	}
}

/* ------------------------------------------------------------ */
/*				Player two movement								*/
/* ------------------------------------------------------------ */


void movePlayerTwoRight(u8 *srcDrame, u8 *nextFrames, u32 stride, int pixelToMove){

	playerTwo.oldPosition = playerTwo.newPosition;
	playerTwo.newPosition += 4*pixelToMove;


	nextFrame = dispCtrl.curFrame + 1;
	lastFrame = dispCtrl.curFrame;
	if (nextFrame >= DISPLAY_NUM_FRAMES)
	{
		nextFrame = 0;
	}
	drawNextFrame(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
	//TimerDelay(1000000/fps);
	DisplayChangeFrame(&dispCtrl, nextFrame);
	clearOldFrame(dispCtrl.framePtr[lastFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);

}


void movePlayerTwoLeft(u8 *srcDrame, u8 *nextFrames, u32 stride, int pixelToMove){

	playerTwo.oldPosition = playerTwo.newPosition;
	playerTwo.newPosition -= 4*pixelToMove;

	nextFrame = dispCtrl.curFrame + 1;
	lastFrame = dispCtrl.curFrame;
	if (nextFrame >= DISPLAY_NUM_FRAMES)
	{
		nextFrame = 0;
	}
	drawNextFrame(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
	//TimerDelay(1000000/fps);
	DisplayChangeFrame(&dispCtrl, nextFrame);
	clearOldFrame(dispCtrl.framePtr[lastFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
}

void movePlayerTwoUp(u8 *srcDrame, u8 *nextFrame, u32 stride, int pixelToMove){

	playerTwo.oldPosition = playerTwo.newPosition;
	playerTwo.newPosition -= stride*pixelToMove;

}

void movePlayerTwoDown(u8 *srcDrame, u8 *nextFrame, u32 stride, int pixelToMove){

	playerTwo.oldPosition = playerTwo.newPosition;
	playerTwo.newPosition += stride*pixelToMove;

}



void jumpPlayerTwo(u8 *frame, u32 stride, int jumpHeight){


	for(int i = jumpHeight ; i > 0 ; i--)
	{
		for(int j = 0 ; j < 5 ; j++)
		{
			movePlayerTwoUp(pFrames[dispCtrl.curFrame], pFrames[dispCtrl.curFrame], DEMO_STRIDE, i);
			nextFrame = dispCtrl.curFrame + 1;
			lastFrame = dispCtrl.curFrame;
			if (nextFrame >= DISPLAY_NUM_FRAMES)
			{
				nextFrame = 0;
			}
			drawNextFrame(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
			TimerDelay(500000/fps);
			DisplayChangeFrame(&dispCtrl, nextFrame);
			clearOldFrame(dispCtrl.framePtr[lastFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
		}
	}

	for(int i = 1 ; i <= jumpHeight ; i++)
	{
		for(int j = 0 ; j < 5 ; j++)
		{
			movePlayerTwoDown(pFrames[dispCtrl.curFrame], pFrames[dispCtrl.curFrame], DEMO_STRIDE, i);
			nextFrame = dispCtrl.curFrame + 1;
			lastFrame = dispCtrl.curFrame;
			if (nextFrame >= DISPLAY_NUM_FRAMES)
			{
				nextFrame = 0;
			}
			drawNextFrame(dispCtrl.framePtr[dispCtrl.curFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
			TimerDelay(500000/fps);
			DisplayChangeFrame(&dispCtrl, nextFrame);
			clearOldFrame(dispCtrl.framePtr[lastFrame], dispCtrl.framePtr[nextFrame], dispCtrl.stride, dispCtrl.vMode.width, dispCtrl.vMode.height);
		}
	}
}







void jumpToRightSidePlayerOne(u8 *frame, u32 stride, int jumpHeight){

	u32 f = 0;
	f++;
	if(f > 2)
	{
		frame = 0;
	}
	//int DisplayStart(dispCtrl, frame[f]);
}


int getNextFrame(){
	int next =dispCtrl.curFrame + 1;
	if (nextFrame >= DISPLAY_NUM_FRAMES-1)
	{
		nextFrame = 0;
	}
	return next;
}

void drawNextFrame(u8 *srcFrame, u8 *nextFrame ,u32 stride, u32 width, u32 height){



//Make new frame
	//Player one
	for(int i = 0 ; i < sizeY ; i++)
	{
		for(int j = 0 ; j < (4*sizeX)  ; j+=4)
		{
			nextFrame[playerOne.newPosition + j] = playerOneBlueColor;
			nextFrame[playerOne.newPosition + j + 1] = playerOneGreenColor;
			nextFrame[playerOne.newPosition + j + 2] = playerOneRedColor;
		}
		playerOne.newPosition += stride;
	}
	playerOne.newPosition -= sizeY*stride;

	//Player Two
		for(int i = 0 ; i < sizeY ; i++)
		{
			for(int j = 0 ; j < (4*sizeX)  ; j+=4)
			{
				nextFrame[playerTwo.newPosition + j] = playerTwoBlueColor;
				nextFrame[playerTwo.newPosition + j + 1] = playerTwoGreenColor;
				nextFrame[playerTwo.newPosition + j + 2] = playerTwoRedColor;
			}
			playerTwo.newPosition += stride;
		}
		playerTwo.newPosition -= sizeY*stride;

	Xil_DCacheFlushRange((unsigned int) nextFrame, DEMO_MAX_FRAME);
}

void clearOldFrame(u8 *srcFrame, u8 *nextFrame ,u32 stride, u32 width, u32 height){

//Clear the old frame, for the player, so only the background is created

	//Player one
	for(int i = 0 ; i < sizeY ; i++)
	{
		for(int j = 0 ; j < (4*sizeX)  ; j+=4)
		{
			srcFrame[playerOne.oldPosition + j] = backgroundBlue;
			srcFrame[playerOne.oldPosition + j + 1] = backgroundGreen;
			srcFrame[playerOne.oldPosition + j + 2] = backgroundRed;
		}
		playerOne.oldPosition += stride;
	}
	playerOne.oldPosition -= sizeY*stride;

	//Player Two
	for(int i = 0 ; i < sizeY ; i++)
	{
		for(int j = 0 ; j < (4*sizeX)  ; j+=4)
		{
			srcFrame[playerTwo.oldPosition + j] = backgroundBlue;
			srcFrame[playerTwo.oldPosition + j + 1] = backgroundGreen;
			srcFrame[playerTwo.oldPosition + j + 2] = backgroundRed;
		}
		playerTwo.oldPosition += stride;
	}
	playerTwo.oldPosition -= sizeY*stride;


	Xil_DCacheFlushRange((unsigned int) nextFrame, DEMO_MAX_FRAME);

}



// Old functions to move players without double buffering.
/*




void movePlayerOneDown(u8 *frame, u32 stride, int pixelToMove){

	for(int i = 0 ; i < pixelToMove ; i++)
	{
		for(int j = 0 ; j < sizeX*4  ; j+=4)
		{
			frame[playerOne.newPosition + j] = backgroundBlue;
			frame[playerOne.newPosition + j + 1] = backgroundGreen;
			frame[playerOne.newPosition + j + 2] = backgroundRed;
		}

	playerOne.newPosition += stride;
	}

	playerOne.newPosition += (sizeY*stride) - (pixelToMove*stride);
	for(int i = 0 ; i < pixelToMove ; i++)
	{
		for(int j = 0 ; j < sizeX*4  ; j+=4)
		{
			frame[playerOne.newPosition + j] = playerOneBlueColor;
			frame[playerOne.newPosition + j + 1] = playerOneGreenColor;
			frame[playerOne.newPosition + j + 2] = playerOneRedColor;
		}

	playerOne.newPosition += stride;
	}
	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);

	playerOne.newPosition -= sizeY*stride;

}



void movePlayerOneLeft(u8 *frame, u32 stride, int pixelToMove){

	//Delete old

	playerOne.newPosition += (sizeX*4) - (pixelToMove*4);
	for(int i = 0 ; i < sizeY ; i++)
	{
		for(int j = 0 ; j < (4*sizeX)  ; j+=4)
		{
			frame[playerOne.newPosition + j] = backgroundBlue;
			frame[playerOne.newPosition + j + 1] = backgroundGreen;
			frame[playerOne.newPosition + j + 2] = backgroundRed;
		}
		playerOne.newPosition += stride;
	}

	//Draw new
	playerOne.newPosition -= (stride*sizeY) + (sizeX*4);
	for(int i = 0 ; i < sizeY ; i++)
	{
		for(int j = 0 ; j < (4*sizeX)  ; j+=4)
		{
			frame[playerOne.newPosition + j] = playerOneBlueColor;
			frame[playerOne.newPosition + j + 1] = playerOneGreenColor;
			frame[playerOne.newPosition + j + 2] = playerOneRedColor;
		}

		playerOne.newPosition += stride;
	}
	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
	playerOne.newPosition -= sizeY*stride;

}


void movePlayerOneUp(u8 *frame, u32 stride, int pixelToMove){

	playerOne.newPosition += ((stride*sizeY) - (pixelToMove*stride));

	for(int i = 0 ; i < pixelToMove ; i++)
	{
		for(int j = 0 ; j < sizeX*4  ; j+=4)
		{
			frame[playerOne.newPosition + j] = backgroundBlue;
			frame[playerOne.newPosition + j + 1] = backgroundGreen;
			frame[playerOne.newPosition + j + 2] = backgroundRed;
		}
		playerOne.newPosition += stride;
	}

	//playerOne.position -= ((sizeY+pixelToMove)*stride);

	playerOne.newPosition -= ((sizeY*stride) + (pixelToMove*stride));

	for(int i = 0 ; i < pixelToMove ; i++)
	{
		for(int j = 0 ; j < sizeX*4  ; j+=4)
		{
			frame[playerOne.newPosition + j] = playerOneBlueColor;
			frame[playerOne.newPosition + j + 1] = playerOneGreenColor;
			frame[playerOne.newPosition + j + 2] = playerOneRedColor;
		}

		playerOne.newPosition += stride;
	}
	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
	playerOne.newPosition -= pixelToMove*stride;

}


void movePlayerOneRight(u8 *frame, u32 stride, int pixelToMove){

	//Delete old

	for(int i = 0 ; i < sizeY ; i++)
	{
		for(int j = 0 ; j < 4*pixelToMove ; j+=4)
		{
			frame[playerOne.position + j] = backgroundBlue;
			frame[playerOne.position + j + 1] = backgroundGreen;
			frame[playerOne.position + j + 2] = backgroundRed;
		}
		playerOne.position += stride;
	}

	//Draw new

	playerOne.position += -(stride*sizeY) + (4*sizeX);
	for(int i = 0 ; i < sizeY ; i++)
	{
		for(int j = 0 ; j < (4*pixelToMove)  ; j+=4)
		{
			frame[playerOne.position + j] = playerOneBlueColor;
			frame[playerOne.position + j + 1] = playerOneGreenColor;
			frame[playerOne.position + j + 2] = playerOneRedColor;
		}
		playerOne.position += stride;
	}
	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);

	playerOne.position += -(sizeY*stride) - (sizeX*4)+(4*pixelToMove);
}






void movePlayerTwoUp(u8 *frame, u32 stride, int pixelToMove){

	playerTwo.newPosition += ((stride*sizeY) - (pixelToMove*stride));

	for(int i = 0 ; i < pixelToMove ; i++)
	{
		for(int j = 0 ; j < sizeX*4  ; j+=4)
		{
			frame[playerTwo.newPosition + j] = backgroundBlue;
			frame[playerTwo.newPosition + j + 1] = backgroundGreen;
			frame[playerTwo.newPosition + j + 2] = backgroundRed;
		}
		playerTwo.newPosition += stride;
	}

	//playerOne.position -= ((sizeY+pixelToMove)*stride);

	playerTwo.newPosition -= ((sizeY*stride) + (pixelToMove*stride));

	for(int i = 0 ; i < pixelToMove ; i++)
	{
		for(int j = 0 ; j < sizeX*4  ; j+=4)
		{
			frame[playerTwo.newPosition + j] = playerTwoBlueColor;
			frame[playerTwo.newPosition + j + 1] = playerTwoGreenColor;
			frame[playerTwo.newPosition + j + 2] = playerTwoRedColor;
		}

		playerTwo.newPosition += stride;
	}
	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
	playerTwo.newPosition -= pixelToMove*stride;
}


void movePlayerTwoDown(u8 *frame, u32 stride, int pixelToMove){

	for(int i = 0 ; i < pixelToMove ; i++)
	{
		for(int j = 0 ; j < sizeX*4  ; j+=4)
		{
			frame[playerTwo.newPosition + j] = backgroundBlue;
			frame[playerTwo.newPosition + j + 1] = backgroundGreen;
			frame[playerTwo.newPosition + j + 2] = backgroundRed;
		}

	playerTwo.newPosition += stride;
	}

	playerTwo.newPosition += (sizeY*stride) - (pixelToMove*stride);
	for(int i = 0 ; i < pixelToMove ; i++)
	{
		for(int j = 0 ; j < sizeX*4  ; j+=4)
		{
			frame[playerTwo.newPosition + j] = playerTwoBlueColor;
			frame[playerTwo.newPosition + j + 1] = playerTwoGreenColor;
			frame[playerTwo.newPosition + j + 2] = playerTwoRedColor;
		}

	playerTwo.newPosition += stride;
	}
	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);

	playerTwo.newPosition -= sizeY*stride;

}


void movePlayerTwoLeft(u8 *frame, u32 stride, int pixelToMove){
	//Delete old

	playerTwo.newPosition += (sizeX*4) - (pixelToMove*4);
	for(int i = 0 ; i < sizeY ; i++)
	{
		for(int j = 0 ; j < (4*sizeX)  ; j+=4)
		{
			frame[playerTwo.newPosition + j] = backgroundBlue;
			frame[playerTwo.newPosition + j + 1] = backgroundGreen;
			frame[playerTwo.newPosition + j + 2] = backgroundRed;
		}
		playerTwo.newPosition += stride;
	}

	//Draw new
	playerTwo.newPosition -= (stride*sizeY) + (sizeX*4);
	for(int i = 0 ; i < sizeY ; i++)
	{
		for(int j = 0 ; j < (4*sizeX)  ; j+=4)
		{
			frame[playerTwo.newPosition + j] = playerTwoBlueColor;
			frame[playerTwo.newPosition + j + 1] = playerTwoGreenColor;
			frame[playerTwo.newPosition + j + 2] = playerTwoRedColor;
		}

		playerTwo.newPosition += stride;
	}
	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);
	playerTwo.newPosition -= sizeY*stride;
}


void movePlayerTwoRight(u8 *frame, u32 stride, int pixelToMove){

	//Delete old

	for(int i = 0 ; i < sizeY ; i++)
	{
		for(int j = 0 ; j < 4*pixelToMove ; j+=4)
		{
			frame[playerTwo.newPosition + j] = backgroundBlue;
			frame[playerTwo.newPosition + j + 1] = backgroundGreen;
			frame[playerTwo.newPosition + j + 2] = backgroundRed;
		}
		playerTwo.newPosition += stride;
	}

	//Draw new
	playerTwo.newPosition += -(stride*sizeY) + (4*sizeX);
	for(int i = 0 ; i < sizeY ; i++)
	{
		for(int j = 0 ; j < (4*pixelToMove)  ; j+=4)
		{
			frame[playerTwo.newPosition + j] = playerTwoBlueColor;
			frame[playerTwo.newPosition + j + 1] = playerTwoGreenColor;
			frame[playerTwo.newPosition + j + 2] = playerTwoRedColor;
		}
		playerTwo.newPosition += stride;
	}
	Xil_DCacheFlushRange((unsigned int) frame, DEMO_MAX_FRAME);

	playerTwo.newPosition += -(sizeY*stride) - (sizeX*4)+(4*pixelToMove);
}

*/





