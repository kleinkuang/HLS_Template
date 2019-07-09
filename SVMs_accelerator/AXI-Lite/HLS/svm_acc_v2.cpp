// File:    svm_acc_v2.cpp
// Author:  Lei Kuang
// Date:    2019.02.12
// @ Imperial College London

// Version 2
// - Customized Binary Divider without using '/'
// - Reduce the number of CORDIC iterations to 6
// - Add debug support, mode defined by #define DEBUG_MODE 9
// - Add Interface AXI-Lite

// ----------------------------------------------------------------
// HLS IP Header
// ----------------------------------------------------------------
#include "svm_acc_v2.h"
#include "ap_sv.h"
#include "ap_alpha.h"

// ----------------------------------------------------------------
// Export Variable for Debug
// ----------------------------------------------------------------
unsigned int hw_debug_cnt = 0;

double hw_dp_sum		[SVs_N*TEST_N]	;
double hw_cordic_tanh	[SVs_N*TEST_N]	;
double hw_sum_acc		[SVs_N*TEST_N]	;
double hw_svm_sum						;

void svm_acc
(
	svm_vec_t 	ap_vec		[VEC_D]			,
	svm_res_t 	*ap_out
)
{
	// AXI-Lite Interface
	#pragma HLS INTERFACE s_axilite port=ap_vec bundle=HLS_SVM
	#pragma HLS INTERFACE s_axilite port=ap_out bundle=HLS_SVM
	#pragma HLS INTERFACE s_axilite port=return bundle=HLS_SVM

	// Partition
	#pragma HLS ARRAY_PARTITION variable=ap_vec complete dim=1
	#pragma HLS ARRAY_PARTITION variable=ap_SVs cyclic factor=240 dim=1
	#pragma HLS ARRAY_PARTITION variable=ap_alpha cyclic factor=15 dim=1

	// Computation Loop
	svm_bias_t sum = 0.0;

	SVM_SVs_LOOP:
	for(unsigned int sv_cnt=0; sv_cnt<SVs_N; sv_cnt++)
	{
		#pragma HLS UNROLL factor=15
		#pragma HLS PIPELINE
		// ----------------------------------------------------------------
		// Dot product 16D x 16D
		// ----------------------------------------------------------------
		dp_sum_t dp_sum;
		dot_product(&ap_vec[0], &ap_SVs[sv_cnt<<4], &dp_sum);

		// Debug
		#if(DEBUG_MODE==0)
			hw_dp_sum[hw_debug_cnt++] = (double)dp_sum;
		#endif

		// Multiply 2 through simply shift to left
		// - Number of fractional bits decreases by 1 due to the shift
		cordic_in_t tanh_in = dp_sum << 1;

		// ----------------------------------------------------------------
		// CORDIC algorithm for hyperbolic functions
		// ----------------------------------------------------------------
		cordic_out_t tanh_out;
		cordic_tanh(tanh_in, &tanh_out);

		// Debug
		#if(DEBUG_MODE==1)
			hw_cordic_tanh[hw_debug_cnt++] = (double)tanh_out;
		#endif

		// ----------------------------------------------------------------
		// Sum accumulation
		// ----------------------------------------------------------------
		svm_bias_t pro = ap_alpha[sv_cnt] * tanh_out;
		#pragma HLS RESOURCE variable=pro core=Mul_LUT

		sum = sum + pro;

		// Debug
		#if(DEBUG_MODE==2)
			hw_sum_acc[hw_debug_cnt++] = (double)sum;
		#endif
	}

	// ----------------------------------------------------------------
	// Biasing
	// ----------------------------------------------------------------
	sum = sum + ap_bias;

	// ----------------------------------------------------------------
	// Classification
	// ----------------------------------------------------------------
	// MSB of a signed number indicates the sign
	//*ap_out = sum >=0 ? 0: 1;
	//(*ap_out).range(0,0) = sum.range(22,22);
	*ap_out = sum.is_neg();

	// External Debug Variable
	#if(DEBUG_MODE==9)
		hw_svm_sum = (double)sum;
	#endif
}

