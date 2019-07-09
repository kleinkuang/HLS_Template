// ----------------------------------------------------------------
// Xilinx SDK Headers
// ----------------------------------------------------------------
#include <stdio.h>
#include "xil_printf.h"

// ----------------------------------------------------------------
// ZYNQ and DMA Headers
// ----------------------------------------------------------------
#include "xaxidma.h"
#include "xparameters.h"
#include "xtime_l.h"

// ----------------------------------------------------------------
// Data for SVM
// ----------------------------------------------------------------
#include "ap_testData_v3.h"

// ----------------------------------------------------------------
// Defined Variable
// ----------------------------------------------------------------
#define TEST_N		2000			// Number of test vectors
									// - Each test data is unsigned int type
									// - which contains two 14-bit vector items

#define BYTE_N		TEST_N*8*4		// - Each testData is 4-byte
									// - Each testData contains 2 vector elements
									// - Eight testData contain a vector of 16 elements
static XAxiDma 	AxiDma;

static u8 		hw_label	[TEST_N];

int main(void)
{

	// ----------------------------------------------------------------
	// Overall Information
	// ----------------------------------------------------------------
	xil_printf("----------------------------------------------------------------\n");
	xil_printf("SVM Accelerator V3 for ADSD\n");
	xil_printf("----------------------------------------------------------------\n");
	xil_printf("Designed by Lei Kuang (lk16)\n\n");
	xil_printf("----------------------------------------------------------------\n");
	xil_printf("Interface:\n");
	xil_printf("- AXI Stream + Polling\n");
	xil_printf("HLS Synthesis Report:\n");
	xil_printf("- Latency:\t95\n");
	xil_printf("- Interval:\t85\n");
	xil_printf("----------------------------------------------------------------\n");

	// ----------------------------------------------------------------
	// Initialize DMA engine
	// ----------------------------------------------------------------
	int Status;
	XAxiDma_Config *Config;
	Config = XAxiDma_LookupConfig(XPAR_AXIDMA_0_DEVICE_ID);
	if (!Config) {
		xil_printf("No config found for %d\r\n", XPAR_AXIDMA_0_DEVICE_ID);
		return XST_FAILURE;
	}

	Status = XAxiDma_CfgInitialize(&AxiDma, Config);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", Status);
		return XST_FAILURE;
	}

	if(XAxiDma_HasSg(&AxiDma)){
		xil_printf("Device configured as SG mode \r\n");
		return XST_FAILURE;
	}

	/* Disable all interrupts before setup */
	XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

	// ----------------------------------------------------------------
	// HLS IP
	// ----------------------------------------------------------------
	xil_printf("start...\n\n");
	XTime tStart, tEnd;
	XTime_GetTime(&tStart);

	// Flush the data before the DMA transfer, in case the Data Cache is enabled
	Xil_DCacheFlushRange((u32)ap_testData, BYTE_N);

	// Stream testData to HLS IP
	Status = XAxiDma_SimpleTransfer(&AxiDma,(u32)ap_testData, BYTE_N, XAXIDMA_DMA_TO_DEVICE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Read back SVM labels

	Status = XAxiDma_SimpleTransfer(&AxiDma,(u32)hw_label, TEST_N, XAXIDMA_DEVICE_TO_DMA);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	// Poll the AXI DMA core
	do
	{
		Status = XAxiDma_Busy(&AxiDma, XAXIDMA_DEVICE_TO_DMA);
	} while(Status);

	// Invalidate the read data before checking the data, in case the Data Cache is enabled
	Xil_DCacheInvalidateRange((u32)hw_label, TEST_N);

	XTime_GetTime(&tEnd);

	printf("Output took %llu clock cycles.\n", 2*(tEnd - tStart));
	printf("Output took %.2f us.\n",
           1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND/1000000));

	/*for(int i=0; i<ARRAY_LENGTH/8; i++)
	{
		xil_printf("%d\n", RxBuffer[i]);
	}*/

	if(TEST_N==2000)
	{
		double error_rate = 0.0;
		unsigned int error_cnt = 0;
		unsigned int confusion[2][2] = {{0, 0}, {0, 0}};

		for (int i=0; i<2000; i++)
		{
			confusion[testDataLabel[i]][hw_label[i]]++;
			if(testDataLabel[i]!=hw_label[i])
				error_cnt++;
		}

		error_rate = error_cnt/2000.0;

		printf("Classification Error Rate: %f\n\n", error_rate);

		xil_printf("Confusion Matrix:\n");
		xil_printf("%-8s%-8s\n", "Golden", "Res");
		xil_printf("%-8d%-8d\n", confusion[0][0], confusion[0][1]);
		xil_printf("%-8d%-8d\n", confusion[1][0], confusion[1][1]);
	}

	xil_printf("Successful !\n");

	return XST_SUCCESS;
}
