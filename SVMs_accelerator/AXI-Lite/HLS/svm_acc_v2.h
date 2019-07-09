// File:    svm_acc_v2.h
// Author:  Lei Kuang
// Date:    2019.02.10
// @ Impeial College London

// --------------------------------
// Xilinx Headers
// --------------------------------
#include "ap_int.h"
#include "ap_fixed.h"

// --------------------------------
// Define Variables
// --------------------------------
#define VEC_D 		16		// Dimension of Vectors
#define SVs_N 		1050	// Number of SVs
#define CORDIC_N 	6		// Number of CORDIC Iterations
#define TEST_N		2000	// Number of test data

// --------------------------------
// Export Variable for Debug
// --------------------------------
#define DEBUG_MODE 9
extern double hw_dp_sum			[SVs_N*TEST_N]	;		// 0
extern double hw_cordic_tanh	[SVs_N*TEST_N]	;		// 1
extern double hw_sum_acc		[SVs_N*TEST_N]	;		// 2
extern double hw_svm_sum					;			// 9
														// Check Label with SW

// --------------------------------
// Arbitrary Precision
// --------------------------------
// ap_[u]fixed<W,I,Q,O,N>
// - W:	Word length in bits
// - I:	The number of bits used to represent the integer value

// Top Function I/O
typedef ap_fixed<14,4, 			AP_RND> 	svm_vec_t;		// (-8, 8)
typedef ap_fixed<14,4, 			AP_RND> 	svm_SVs_t;		// (-8, 8)
typedef ap_fixed<CORDIC_N*2,5,	AP_RND> 	svm_alpha_t;	// (-16, 16)
typedef ap_fixed<23,11> 					svm_bias_t;		// (-1024, 1024)
typedef ap_ufixed<1,1> 						svm_res_t;

// Sub Function I/O
#define DP_SUM_FP 12									// Number of fractional bits for dot product result
typedef	ap_fixed<11+DP_SUM_FP,11>		dp_sum_t;
typedef ap_fixed<11+DP_SUM_FP,12>		cordic_in_t;	// cordic_in_t = dp_sum_t << 1
typedef ap_fixed<CORDIC_N+2, 2>			cordic_out_t;

// --------------------------------
// Function Prototype
// --------------------------------
void dot_product	(	svm_vec_t	ap_vec[VEC_D],	svm_SVs_t	ap_SVs[VEC_D],	dp_sum_t	*dp_sum_out		);
void cordic_tanh	(	cordic_in_t	tanh_in, 		cordic_out_t *tanh_out									);
void svm_acc		( 	svm_vec_t*, 				svm_res_t*												);

