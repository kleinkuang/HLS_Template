// File:    svm_acc_tb_v2.cpp
// Author:  Lei Kuang
// Date:    2019.02.15
// @ Imperial College London

//#define CONVERT_TESTDATA

// ----------------------------------------------------------------
// C++ Headers
// ----------------------------------------------------------------
#include <iostream>
#include <iomanip>

// ----------------------------------------------------------------
// HLS IP Headers
// ----------------------------------------------------------------
#include "svm_acc_v3.h"
#include "sv.h"
#include "alpha.h"

#ifdef CONVERT_TESTDATA
	#include "testData.h"
#else
	#include "ap_testData.h"
#endif

// ----------------------------------------------------------------
// C++ Name Space
// ----------------------------------------------------------------
using namespace std;

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
	#ifdef CONVERT_TESTDATA

	svm_vec_t ap_testData[2000*VEC_D];

	for(int i=0;i<2000*VEC_D;i++)
		ap_testData[i] = testData[i];

	// Convert ap_fix to unsigned int for Xilinx SDK
	for(int i=0;i<2000*VEC_D/2;i++)
	{
		unsigned int data_uint = 0;
		data_uint = (int)ap_testData[i*2].range(13,0);
		data_uint = data_uint | ((int)ap_testData[i*2+1].range(13,0)) << 16;
		printf("0x%-8x,\n", data_uint);
	}

	return 0;

	#else

	// - Label
	int hw_label [2000];

	// ----------------------------------------------------------------
	// Testing loop
	// ----------------------------------------------------------------
	svm_vec_stream_t 	ap_vec_stream;
	svm_res_stream_t 	ap_out_stream;

	int test_n = 2000;

	// Stream Data -> FIFO
	for(int i=0; i< test_n * 8; i++)
	{
		svm_vec_last_t ap_vec_temp;

		ap_vec_temp.data = ap_testData[i];
		ap_vec_temp.last = i==2000*8-1 ? 1 : 0;

		ap_vec_stream << ap_vec_temp;
	}

	// FIFO -> IP
	while(!ap_vec_stream.empty())
	{
		svm_acc(ap_vec_stream, ap_out_stream);
	}

	// FIFO -> Stream Data
	for(int i=0; i< test_n; i++)
	{
		svm_res_last_t res_temp;
		ap_out_stream >> res_temp;
		hw_label[i] = (int)res_temp.data;
	}

	// ----------------------------------------------------------------
	// Summary statistics
	// ----------------------------------------------------------------
	if(test_n==2000)
	{
		double error_rate = 0.0;
		unsigned int error_cnt = 0;
		unsigned int confusion[2][2] = {0, 0, 0, 0};

		for (int i=0; i<2000; i++)
		{
			confusion[testDataLabel[i]][hw_label[i]]++;
			if(testDataLabel[i]!=hw_label[i])
				error_cnt++;
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

	#endif
}
