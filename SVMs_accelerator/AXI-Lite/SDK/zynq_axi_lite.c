// ----------------------------------------------------------------
// Xilinx SDK Headers
// ----------------------------------------------------------------
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"

// ----------------------------------------------------------------
// ZYNQ and HLS Headers
// ----------------------------------------------------------------
#include "xparameters.h" 	// Parameter definitions for processor peripherals
#include "xscugic.h" 		// Processor interrupt controller device driver
#include "xsvm_acc.h" 		// Device driver for HLS HW block
#include "xtime_l.h"		// Time Measurement for bare metal

// ----------------------------------------------------------------
// Data for SVM
// ----------------------------------------------------------------
#include "ap_testData.h"

// ----------------------------------------------------------------
// Data for SVM
// ----------------------------------------------------------------
XSvm_acc SvmAcc;	// HLS macc HW instance
XScuGic ScuGic;		//Interrupt Controller Instance

// Define global variables to interface with the interrupt service routine (ISR).
volatile static int RunSvmAcc = 0;
volatile static int ResultAvailSvmAcc = 0;

// Define a function to wrap all run-once API initialization function calls for the HLS block.
int hls_svm_acc_init(XSvm_acc *svm_accPtr)
{
	XSvm_acc_Config *cfgPtr;
	int status;
	cfgPtr = XSvm_acc_LookupConfig(XPAR_XSVM_ACC_0_DEVICE_ID);
	if (!cfgPtr)
	{
		xil_printf("ERROR: Lookup of accelerator configuration failed.\n\r");
		return XST_FAILURE;
	}

	status = XSvm_acc_CfgInitialize(svm_accPtr, cfgPtr);

	if (status != XST_SUCCESS)
	{
		xil_printf("ERROR: Could not initialize accelerator.\n\r");
		return XST_FAILURE;
	}

	return status;
}

// Define a helper function to wrap the HLS block API calls required to enable its interrupt and start the block.
void hls_svm_acc_start(void *InstancePtr)
{
	XSvm_acc *pAccelerator = (XSvm_acc *)InstancePtr;
	XSvm_acc_InterruptEnable(pAccelerator,1);
	XSvm_acc_InterruptGlobalEnable(pAccelerator);
	XSvm_acc_Start(pAccelerator);
}

void hls_svm_acc_isr(void *InstancePtr)
{
	XSvm_acc *pAccelerator = (XSvm_acc *)InstancePtr;
	//Disable the global interrupt
	XSvm_acc_InterruptGlobalDisable(pAccelerator);
	//Disable the local interrupt
	XSvm_acc_InterruptDisable(pAccelerator,0xffffffff);
	// clear the local interrupt
	XSvm_acc_InterruptClear(pAccelerator,1);
	ResultAvailSvmAcc = 1;
	// restart the core if it should run again
	if(RunSvmAcc)
	{
		hls_svm_acc_start(pAccelerator);
	}
}

int setup_interrupt()
{
	//This functions sets up the interrupt on the Arm
	int result;
	XScuGic_Config *pCfg = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
	if (pCfg == NULL)
	{
		xil_printf("Interrupt Configuration Lookup Failed\n\r");
		return XST_FAILURE;
	}

	result = XScuGic_CfgInitialize(&ScuGic,pCfg,pCfg->CpuBaseAddress);

	if(result != XST_SUCCESS)
	{
		return result;
	}

	// self-test
	result = XScuGic_SelfTest(&ScuGic);
	if(result != XST_SUCCESS)
	{
		return result;
	}

	// Initialize the exception handler
	Xil_ExceptionInit();

	// Register the exception handler
	//xil_printf("Register the exception handler\n\r");
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler,&ScuGic);

	//Enable the exception handler
	Xil_ExceptionEnable();

	// Connect the Adder ISR to the exception table
	//xil_printf("Connect the Adder ISR to the Exception handler table\n\r");
	result = XScuGic_Connect(&ScuGic, XPAR_FABRIC_SVM_ACC_0_INTERRUPT_INTR, (Xil_InterruptHandler)hls_svm_acc_isr,&SvmAcc);

	if(result != XST_SUCCESS)
	{
		return result;
	}

	//xil_printf("Enable the Adder ISR\n\r");
	XScuGic_Enable(&ScuGic,XPAR_FABRIC_SVM_ACC_0_INTERRUPT_INTR);

	return XST_SUCCESS;
}

int main()
{
	init_platform();

	// ----------------------------------------------------------------
	// Overall Information
	// ----------------------------------------------------------------
	xil_printf("----------------------------------------------------------------\n");
	xil_printf("SVM Accelerator V2 for ADSD\n");
	xil_printf("----------------------------------------------------------------\n");
	xil_printf("Designed by Lei Kuang (lk16)\n\n");
	xil_printf("----------------------------------------------------------------\n");
	xil_printf("Interface:\n");
	xil_printf("- AXI Lite + Interrupt\n");
	xil_printf("HLS Synthesis Report:\n");
	xil_printf("- Latency:\t1063\n");
	xil_printf("- Interval:\t1063\n");
	xil_printf("----------------------------------------------------------------\n");

	int status;
	//Setup the matrix mult
	status = hls_svm_acc_init(&SvmAcc);
	if(status != XST_SUCCESS)
	{
		xil_printf("HLS peripheral setup failed\n\r");
		return 1;
	}

	//Setup the interrupt
	status = setup_interrupt();
	if(status != XST_SUCCESS)
	{
		xil_printf("Interrupt setup failed\n\r");
		return 1;
	}

	// set the input parameters of the HLS block
	xil_printf("\nStart...\n\n");
	XTime tStart, tEnd;
	XTime_GetTime(&tStart);

	unsigned int res_hw [2000];

	for(int i=0; i<2000; i++)
	{
		XSvm_acc_Set_ap_vec_0_V		(&SvmAcc, ap_testData[(i<<4)+0]);
		XSvm_acc_Set_ap_vec_1_V		(&SvmAcc, ap_testData[(i<<4)+1]);
		XSvm_acc_Set_ap_vec_2_V		(&SvmAcc, ap_testData[(i<<4)+2]);
		XSvm_acc_Set_ap_vec_3_V		(&SvmAcc, ap_testData[(i<<4)+3]);
		XSvm_acc_Set_ap_vec_4_V		(&SvmAcc, ap_testData[(i<<4)+4]);
		XSvm_acc_Set_ap_vec_5_V		(&SvmAcc, ap_testData[(i<<4)+5]);
		XSvm_acc_Set_ap_vec_6_V		(&SvmAcc, ap_testData[(i<<4)+6]);
		XSvm_acc_Set_ap_vec_7_V		(&SvmAcc, ap_testData[(i<<4)+7]);
		XSvm_acc_Set_ap_vec_8_V		(&SvmAcc, ap_testData[(i<<4)+8]);
		XSvm_acc_Set_ap_vec_9_V		(&SvmAcc, ap_testData[(i<<4)+9]);
		XSvm_acc_Set_ap_vec_10_V	(&SvmAcc, ap_testData[(i<<4)+10]);
		XSvm_acc_Set_ap_vec_11_V	(&SvmAcc, ap_testData[(i<<4)+11]);
		XSvm_acc_Set_ap_vec_12_V	(&SvmAcc, ap_testData[(i<<4)+12]);
		XSvm_acc_Set_ap_vec_13_V	(&SvmAcc, ap_testData[(i<<4)+13]);
		XSvm_acc_Set_ap_vec_14_V	(&SvmAcc, ap_testData[(i<<4)+14]);
		XSvm_acc_Set_ap_vec_15_V	(&SvmAcc, ap_testData[(i<<4)+15]);

		while (!XSvm_acc_IsReady(&SvmAcc))
		{
		}

		hls_svm_acc_start(&SvmAcc);

		#ifndef INTERRUPT_MODE
			// Interrupt
			while(!ResultAvailSvmAcc)
			{

			}

			res_hw[i] = XSvm_acc_Get_ap_out_V(&SvmAcc);
		#else
			// Polling
			hls_svm_acc_start(&SvmAcc);
			do
			{
				res_hw[i] = XSvm_acc_Get_ap_out_V(&SvmAcc);
			} while (!XSvm_acc_IsReady(&SvmAcc));
		#endif
	}

	XTime_GetTime(&tEnd);

	// ----------------------------------------------------------------
	// Time
	// ----------------------------------------------------------------
	printf("Output took %llu clock cycles\n", 2*(tEnd - tStart));
	printf("Output took %.2f us\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND/1000000));

	// ----------------------------------------------------------------
	// Summary statistics
	// ----------------------------------------------------------------
	#ifndef CHECK_CONFUSION_MATRIX

		double error_rate = 0.0;
		unsigned int error_cnt=0;
		unsigned int confusion[2][2] = {{0,0}, {0,0}};

		for (int i=0; i<2000; i++)
		{
			confusion[testDataLabel[i]][res_hw[i]]++;

			if(testDataLabel[i]!=res_hw[i])
				error_cnt++;
		}

		error_rate = error_cnt/2000.0;

		printf("Classification Error Rate: %f\n", (float)error_rate);
		xil_printf("Confusion Matrix:\n");
		xil_printf("%-8s%-8s\n", "Golden", "Res");
		xil_printf("%-8d%-8d\n", confusion[0][0], confusion[0][1]);
		xil_printf("%-8d%-8d\n", confusion[1][0], confusion[1][1]);

	#endif

	cleanup_platform();

	return status;
}

