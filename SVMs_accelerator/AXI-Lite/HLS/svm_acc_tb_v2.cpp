// File:    svm_acc_tb_v2.cpp
// Author:  Lei Kuang
// Date:    2019.02.15
// @ Imperial College London

// ----------------------------------------------------------------
// C++ Headers
// ----------------------------------------------------------------
#include <iostream>
#include <iomanip>

// ----------------------------------------------------------------
// HLS IP Headers
// ----------------------------------------------------------------
#include "svm_acc_v2.h"
#include "sv.h"
#include "alpha.h"
#include "testData.h"

// ----------------------------------------------------------------
// C++ Name Space
// ----------------------------------------------------------------
using namespace std;

// ----------------------------------------------------------------
// Function Declaration
// ----------------------------------------------------------------
int classify_golden(double x[16]);

// ----------------------------------------------------------------
// Data conversion for AP
// ----------------------------------------------------------------

unsigned int sw_debug_cnt = 0;
unsigned int error_cnt =0;
double sw_dp_sum		[SVs_N*TEST_N]	;
double sw_cordic_tanh	[SVs_N*TEST_N]	;
double sw_sum_acc		[SVs_N*TEST_N]	;
double sw_svm_sum						;

// ----------------------------------------------------------------
// Data conversion for AP
// ----------------------------------------------------------------
int main()
{
	printf("\n\n----------------------------------------------------------------\n\n");

	double error=0.0;
	double max_error=0.0, min_error=0.0;

	// ----------------------------------------------------------------
	// Data conversion for AP
	// ----------------------------------------------------------------
	// - Test vectors
	svm_vec_t ap_testData[2000*VEC_D];

	for(int i=0;i<2000*VEC_D;i++)
		ap_testData[i] = testData[i];

	// Convert ap_fix to int for Xilinx SDK
	#ifdef CONVERT_TESTDATA

	for(int i=0;i<2000*VEC_D;i++)
	{
		printf("0x%x,\n", (int)ap_testData[i].range(13,0));
	}
	return 0;

	#endif

	// - Label
	int hw_label [2000];

	// ----------------------------------------------------------------
	// Testing loop
	// ----------------------------------------------------------------
	int loop_end = 2000;
	for (int i=0; i<loop_end; i++)
    {
		int sw_res, hw_res;

		// Call the function
		sw_res = classify_golden(&testData[i<<4]);

		// Call hls ip
		svm_res_t svm_res;
		svm_acc (&ap_testData[i<<4], &svm_res);
		hw_res = (int)svm_res;


		#if(DEBUG_MODE==0)
			for(int j=0; j<SVs_N; j++)
			{
				printf("%4d SW:dp  :\t%21.16f\t", i, sw_dp_sum[j]);
				printf("HW:dp  :\t%21.16f\t", hw_dp_sum[j]);
				error = hw_dp_sum[j] - sw_dp_sum[j];

				// Error Estimation
				printf("E: %10f\n", error);
				if(max_error<error)
					max_error = error;
				if(min_error>error)
					min_error = error;
			}
		#endif

		#if(DEBUG_MODE==1)
			for(int j=0; j<SVs_N; j++)
			{
				printf("%4d SW:tanh:\t%21.16f\t", i, sw_cordic_tanh[j]);
				printf("HW:tanh:\t%21.16f\t", hw_cordic_tanh[j]);
				error = hw_cordic_tanh[j] - sw_cordic_tanh[j];

				// Error Estimation
				printf("E: %10f\n", error);
				if(max_error<error)
					max_error = error;
				if(min_error>error)
					min_error = error;
			}
		#endif

		#if(DEBUG_MODE==2)
			for(int j=0; j<SVs_N; j++)
			{
				printf("%4d SW:suma:\t%21.16f\t", i, sw_sum_acc[j]);
				printf("HW:suma:\t%21.16f\t", hw_sum_acc[j]);
				error = sw_sum_acc[j] - hw_sum_acc[j];

				// Error Estimation
				printf("E: %10f\n", error);
				if(max_error<error)
					max_error = error;
				if(min_error>error)
					min_error = error;
			}
		#endif

		#if(DEBUG_MODE==9)
			printf("%4d SW:sum :\t%21.16f\t", i, sw_svm_sum);
			printf("HW:sum :\t%21.16f\t", hw_svm_sum);
			error = hw_svm_sum - sw_svm_sum;

			// Error Estimation
			printf("E: %10f\n", error);
			if(max_error<error)
				max_error = error;
			if(min_error>error)
				min_error = error;
		#endif

		// Check Golden Label
		#if(DEBUG_MODE==10)
			printf("%4d G:%d\t", i, testDataLabel[i]);
			printf("S:%d\t", sw_res);
			printf("H:%d\t", hw_res);

			if(testDataLabel[i]!=sw_res)
				printf("X");
			else
				printf(" ");

			printf(" ");

			if(testDataLabel[i]!=hw_res)
				printf("X");
			else
				printf(" ");

			printf("\n");
		#endif

		#ifndef SW_RES_CHECK
			if(hw_res!=sw_res)
				return 1;
		#endif

		// Check Classification Result
		hw_label[i] = hw_res;
		if(testDataLabel[i]!=hw_res)
			error_cnt = error_cnt + 1;
			// Return with Error
			if(error_cnt>170)
			{
				printf("Summary:\nMax:%10f\nMin:%10f\n", max_error, min_error);
				printf("Error Cnt:%d\n", error_cnt);
				return 1;
			}
	}

	#if(DEBUG_MODE!=10)
		printf("Summary:\nMax:%10f\nMin:%10f\n", max_error, min_error);
	#endif

	printf("Error Cnt:%d\n", error_cnt);

	// ----------------------------------------------------------------
	// Summary statistics
	// ----------------------------------------------------------------
	if(loop_end==2000)
	{
		double error_rate = 0.0;
		int confusion[2][2] = {0, 0, 0, 0};

		for (int i=0; i<2000; i++){
			confusion[testDataLabel[i]][hw_label[i]]++;
		}

		error_rate = error_cnt/2000.0;

		printf("\n\nClassification Error Rate: %f\n\n", error_rate);

		printf("Confusion Matrix:\n");
		printf("%-8s%-8s\n", "Golden", "Res");
		printf("%-8d%-8d\n", confusion[0][0], confusion[0][1]);
		printf("%-8d%-8d\n", confusion[1][0], confusion[1][1]);
	}

	/*
	 * Classification Error Rate: 0.085000
	 * Confusion Matrix:
	 * Golden  Res
	 * 1826    98
	 * 72      4
	*/

	printf("\n\n----------------------------------------------------------------\n\n");
	return 0;
}

int classify_golden(double x[16]){
	int i, j;
	double sum = 0.0;
	double tanh_value;

	for (i=0; i<SVs_N; i++)
	{
		// Calculate the dot product
		double temp= 0.0;

		for (j=0; j<16; j++)
		{
			temp = temp + x[j]*SVs[i*16+j];
		}

		// Debug
		#if(DEBUG_MODE==0)
			sw_dp_sum[sw_debug_cnt++] = temp;
		#endif

		tanh_value = tanh(2*temp);

		// Debug
		#if(DEBUG_MODE==1)
			sw_cordic_tanh[sw_debug_cnt++] = tanh_value;
		#endif

		sum = sum + alpha[i] * tanh_value;

		// Debug
		#if(DEBUG_MODE==2)
			sw_sum_acc[sw_debug_cnt++] = (double)sum;
		#endif
	}

	sum = sum + bias;

	// Debug
	#if(DEBUG_MODE==9)
		sw_svm_sum = sum;
	#endif

	return sum>=0 ? 0 : 1;
}
