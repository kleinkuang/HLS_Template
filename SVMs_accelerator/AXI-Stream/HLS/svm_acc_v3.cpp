// File:    svm_acc_v3.cpp
// Author:  Lei Kuang
// Date:    2019.02.15
// @ Imperial College London

// Version 3
// - Add Interface AXI-Stream

#include "svm_acc_v3.h"

#include "ap_sv.h"
#include "ap_alpha.h"

void svm_acc
(
	svm_vec_stream_t	& ap_vec_stream,
	svm_res_stream_t	& ap_out_stream
)
{
		#pragma HLS DATAFLOW

		// AXI-Stream Interface
		#pragma HLS INTERFACE axis register both port=ap_vec_stream
		#pragma HLS INTERFACE axis register both port=ap_out_stream

		// Partition
		#pragma HLS ARRAY_PARTITION variable=ap_SVs cyclic factor=240 dim=1
		#pragma HLS ARRAY_PARTITION variable=ap_alpha cyclic factor=15 dim=1

		// ----------------------------------------------------------------
		// Read input vector through AXI-Stream interface
		// ----------------------------------------------------------------
		svm_vec_t ap_vec[16];
		#pragma HLS ARRAY_PARTITION variable=ap_vec complete dim=1
		ap_uint<1> last_flag;

		SVM_VEC_LOOP:
		for(int i=0; i<8; i++)
		{
			#pragma HLS PIPELINE
			// FIFO Read
			svm_vec_last_t ap_vec_temp;
			ap_vec_stream >> ap_vec_temp;

			// FIFO Output two vector items
			svm_vec_pack_t ap_vec_x2;
			ap_vec_x2 = ap_vec_temp.data;

			// Assign 32-bit data to two vectors
			ap_vec[i<<1].range(13, 0)		= ap_vec_x2.range(13, 0);
			ap_vec[(i<<1)+1].range(13, 0)	= ap_vec_x2.range(29, 16);

			// AXI Stream Last Flag
			if(i==7)
				last_flag = ap_vec_temp.last;
		}

		// ----------------------------------------------------------------
		// Computation Loop
		// ----------------------------------------------------------------
		svm_bias_t sum = 0.0;

		SVM_SVs_LOOP:
		for(unsigned int sv_cnt=0; sv_cnt<SVs_N; sv_cnt++)
		{
			#pragma HLS PIPELINE
			#pragma HLS UNROLL factor=15
			// ----------------------------------------------------------------
			// Dot product 16D x 16D
			// ----------------------------------------------------------------
			dp_sum_t dp_sum;
			dot_product(&ap_vec[0], &ap_SVs[sv_cnt<<4], &dp_sum);

			// Multiply 2 through simply shift to left
			// - Number of fractional bits decreases by 1 due to the shift
			cordic_in_t tanh_in = dp_sum << 1;

			// ----------------------------------------------------------------
			// CORDIC algorithm for hyperbolic functions
			// ----------------------------------------------------------------
			cordic_out_t tanh_out;
			cordic_tanh(tanh_in, &tanh_out);

			// ----------------------------------------------------------------
			// Sum accumulation
			// ----------------------------------------------------------------
			svm_bias_t pro = ap_alpha[sv_cnt] * tanh_out;
			#pragma HLS RESOURCE variable=pro core=Mul_LUT

			sum = sum + pro;

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
		svm_res_last_t ap_out;

		ap_out.data = sum.is_neg();
		ap_out.last = last_flag;

		ap_out_stream << ap_out;
}

