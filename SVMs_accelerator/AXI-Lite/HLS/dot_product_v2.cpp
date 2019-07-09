// File:    dot_product_v2.cpp
// Author:  Lei Kuang
// Date:    2019.02.15
// @ Imperial College London

#include "svm_acc_v2.h"

void dot_product
(
		svm_vec_t 	ap_vec[VEC_D],
		svm_SVs_t	ap_SVs[VEC_D],
		dp_sum_t	*dp_sum_out
)
{
	#pragma HLS INLINE

	// ----------------------------------------------------------------
	// Dot product 16D x 16D
	// ----------------------------------------------------------------
	// !!! Lose Precision !!!

	// Input test vectors and support vectors are 16-bit (Use 14-bit here)
	// f4.10 * f4.10 = f7.20
	// USE f7.10 to store the product

	// Unroll the loop manually for MUL_LUT directive
	// Maximize the utilization of DSP for dot product

	ap_fixed<17,7> dp_pro_0,	dp_pro_1,	dp_pro_2,	dp_pro_3	;
	#pragma HLS RESOURCE variable=dp_pro_0 core=Mul_LUT
	ap_fixed<17,7> dp_pro_4,	dp_pro_5,	dp_pro_6,	dp_pro_7	;
	ap_fixed<17,7> dp_pro_8,	dp_pro_9,	dp_pro_10,	dp_pro_11	;
	ap_fixed<17,7> dp_pro_12,	dp_pro_13,	dp_pro_14,	dp_pro_15	;
	#pragma HLS RESOURCE variable=dp_pro_15 core=Mul_LUT

	dp_pro_0  = ap_vec[0]  * ap_SVs[0];
	dp_pro_1  = ap_vec[1]  * ap_SVs[1];
	dp_pro_2  = ap_vec[2]  * ap_SVs[2];
	dp_pro_3  = ap_vec[3]  * ap_SVs[3];
	dp_pro_4  = ap_vec[4]  * ap_SVs[4];
	dp_pro_5  = ap_vec[5]  * ap_SVs[5];
	dp_pro_6  = ap_vec[6]  * ap_SVs[6];
	dp_pro_7  = ap_vec[7]  * ap_SVs[7];
	dp_pro_8  = ap_vec[8]  * ap_SVs[8];
	dp_pro_9  = ap_vec[9]  * ap_SVs[9];
	dp_pro_10 = ap_vec[10] * ap_SVs[10];
	dp_pro_11 = ap_vec[11] * ap_SVs[11];
	dp_pro_12 = ap_vec[12] * ap_SVs[12];
	dp_pro_13 = ap_vec[13] * ap_SVs[13];
	dp_pro_14 = ap_vec[14] * ap_SVs[14];
	dp_pro_15 = ap_vec[15] * ap_SVs[15];

	// f7.10 + f7.12 x 10 = f11.10
	dp_sum_t dp_sum = 0.0;

	dp_sum = 	dp_pro_0 	+ dp_pro_1 	+ dp_pro_2 	+ dp_pro_3 	+
				dp_pro_4 	+ dp_pro_5 	+ dp_pro_6 	+ dp_pro_7 	+
				dp_pro_8 	+ dp_pro_9 	+ dp_pro_10 + dp_pro_11 +
				dp_pro_12 	+ dp_pro_13 + dp_pro_14 + dp_pro_15		;

	/*
	ap_ufixed<4,4> i=0;
	ap_fixed<20,8> dp_pro;

	DOT_PRODUCT_LOOP:
	for(unsigned int i=0; i<VEC_D; i++)
	{
		#pragma HLS UNROLL
		dp_pro = ap_vec[i] * ap_SVs[i];
		dp_sum = dp_sum + dp_pro;

	}
	*/

	*dp_sum_out = dp_sum;
}
