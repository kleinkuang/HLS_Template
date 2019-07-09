// File:    cordic_tanh_v2.cpp
// Author:  Lei Kuang
// Date:    2019.02.15
// @ Imperial College London

#include "svm_acc_v2.h"

// ----------------------------------------------------------------
// CORDIC Look-up Table
// ----------------------------------------------------------------
// The precision of the LUT depends on the CORDIC_N

ap_ufixed <CORDIC_N,0> ATANH_LUT [CORDIC_N] =
{
    0.5493061443340548,	// 1
    0.2554128118829954,	// 2
    0.1256572141404530,	// 3
    0.0625815714770030,	// 4
    0.0312601784906670,	// 5
    0.0156262717520522,	/*// 6
    0.0078126589515404, // 7
    0.0039062698683968,	// 8
    0.0019531274835325,	// 9
    0.0009765628104410,	// 10
    0.0004882812888051,	// 11
    0.0002441406298506,	// 12
    0.0001220703131063, // 13
    0.0000610351563258,	// 14
    0.0000305175781345,	// 15
    0.0000152587890637	// 16
    */
};

ap_ufixed<8, 5>  SINH_OPT_LUT [4] =
{
	0,
	1.1752011936438014,
	3.6268604078470190,
	10.0178749274099026,
	//27.2899171971277497,
	//74.2032105777887523,
	//201.7131573702792195,
};

ap_ufixed<8, 5>  COSH_OPT_LUT [4] =
{
	1,
	1.5430806348152439,
	3.7621956910836314,
	10.0676619957777653,
	//27.3082328360164865,
	//74.2099485247878476,
	//201.7156361224558907,
};

void cordic_tanh
(
	cordic_in_t 	theta,
	cordic_out_t *tanh_out
)
{
	#pragma HLS INLINE
	#pragma HLS ARRAY_PARTITION variable=COSH_OPT_LUT complete dim=1
	#pragma HLS ARRAY_PARTITION variable=SINH_OPT_LUT complete dim=1
	#pragma HLS ARRAY_PARTITION variable=ATANH_LUT complete dim=1

	// ----------------------------------------------------------------
	// CORDIC algorithm for hyperbolic functions
	// ----------------------------------------------------------------
	// Optimization for large input value
	// - MSB of a 2's complement number indicates the sign
	ap_ufixed<1,1> theta_sign = theta.is_neg();

	// - Obtain absolute value of theta
	// - Withdraw sign bit
	ap_ufixed<11+DP_SUM_FP-1,11> theta_abs;

	if(theta_sign==0)
		theta_abs = theta;
	else
		theta_abs = -theta;

	// Separate integer and fractional part as CORDIC only accepts a value <1
	ap_ufixed <2,2> theta_int;
	ap_fixed <2+DP_SUM_FP-1,2> theta_fra = 0.0;	// !!! Store CORDIC z value which could be negative (-2, 2)

	// - Integer Saturation
	theta_int.range(1,0) = theta_abs.range(11+DP_SUM_FP-2, DP_SUM_FP-1) < 4 ? theta_abs.range(13, DP_SUM_FP-1) : 3;

	theta_fra.range(DP_SUM_FP-2,0) = theta_abs.range(DP_SUM_FP-2,0);

	// ONLY Calculate the fractional part of the input
	// - input is limited by a range of [0,1)
	// - sinh(1) = 1.1752
	// - cosh(1) = 1.5431
	ap_ufixed <1,1> dir;
	ap_ufixed <CORDIC_N,1> x = 1.2051363583996952;
	ap_ufixed <CORDIC_N,1> y = 0;
	ap_ufixed <CORDIC_N,1> x_next, y_next;

	CORDIC_ITERATION_LOOP:
	for(unsigned int i=0; i<CORDIC_N; i++)
	{
		#pragma HLS UNROLL
		dir = theta_fra>=0 ? 0 : 1;

		if(dir==0)
		{
			x_next = x + (y >> (i+1));
			y_next = y + (x >> (i+1));
			theta_fra = theta_fra - ATANH_LUT[i];
		}
		else
		{
			x_next = x - (y >> (i+1));
			y_next = y - (x >> (i+1));
			theta_fra = theta_fra + ATANH_LUT[i];
		}

		x = x_next;	// cosh
		y = y_next;	// sinh

	}

	// Calculate tanh with theta_int and theta_fra
	ap_ufixed<CORDIC_N*2, 7> sinh;
	ap_ufixed<CORDIC_N*2, 7> cosh;
	ap_ufixed<CORDIC_N*2, 0> tanh_abs = 0;

	// - sinh(theta_abs) and cosh(theta_abs)

	// - Timing Failed !
	//sinh = x*SINH_OPT_LUT[theta_int] + y*COSH_OPT_LUT[theta_int];
	//cosh = x*COSH_OPT_LUT[theta_int] + y*SINH_OPT_LUT[theta_int];

	ap_ufixed<CORDIC_N*2, 6> xSinh;
	#pragma HLS RESOURCE variable=xSinh core=Mul_LUT
	ap_ufixed<CORDIC_N*2, 6> yCosh;
	#pragma HLS RESOURCE variable=yCosh core=Mul_LUT
	ap_ufixed<CORDIC_N*2, 6> xCosh;
	#pragma HLS RESOURCE variable=xCosh core=Mul_LUT
	ap_ufixed<CORDIC_N*2, 6> ySinh;
	#pragma HLS RESOURCE variable=ySinh core=Mul_LUT

	xSinh = x*SINH_OPT_LUT[theta_int];
	yCosh = y*COSH_OPT_LUT[theta_int];
	xCosh = x*COSH_OPT_LUT[theta_int];
	ySinh = y*SINH_OPT_LUT[theta_int];

	sinh = xSinh + yCosh;
	cosh = xCosh + ySinh;

	// Division
	cosh = cosh >> 1;
	CORDIC_DIVISION_LOOP:
	for(unsigned int j=0; j<7; j++)
	{
		#pragma HLS UNROLL
		if(sinh>cosh)
		{
			tanh_abs[CORDIC_N*2-1-j] = 1;
			sinh = (sinh - cosh) << 1;
		}
		else
		{
			tanh_abs[CORDIC_N*2-1-j] = 0;
			sinh = sinh << 1;
		}
	}

	// - tanh(theta)
	cordic_out_t tanh;

	// - add original sign
	if(theta_abs.range(11+DP_SUM_FP-2,11) >= 4)
		if(theta_sign==0)
			tanh = 1;
		else
			tanh = -1;
	else
		if(theta_sign==0)
			tanh = tanh_abs;
		else
			tanh = -tanh_abs;

	// Output
	*tanh_out = tanh;
}
