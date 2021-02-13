#include <math.h>
#include "dvb_t.h"

const fcmplx c_qpsk[4] =
{
    {1,  1},
    {1, -1},
	{-1, 1},
	{-1,-1}
};

const fcmplx c_qam16[16] =
{
	{3, 3},
	{3, 1},
	{1, 3},
	{1, 1},
	{3,-3},
	{3,-1},
	{1,-3},
	{1,-1},
	{-3, 3},
	{-3, 1},
	{-1, 3},
	{-1, 1},
	{-3,-3},
	{-3,-1},
	{-1,-3},
	{-1,-1}
};
const fcmplx c_qam64[64] =
{
	{7, 7},
	{7, 5},
	{5, 7},
	{5, 5},
	{7, 1},
	{7, 3},
	{5, 1},
	{5, 3},
	{1, 7},
	{1, 5},
	{3, 7},
	{3, 5},
	{1, 1},
	{1, 3},
	{3, 1},
	{3, 3},
	{7, -7},
	{7, -5},
	{5, -7},
	{5, -5},
	{7, -1},
	{7, -3},
	{5, -1},
	{5, -3},
	{1, -7},
	{1, -5},
	{3, -7},
	{3, -5},
	{1, -1},
	{1, -3},
	{3, -1},
	{3, -3},
	{-7, 7},
	{-7, 5},
	{-5, 7},
	{-5, 5},
	{-7, 1},
	{-7, 3},
	{-5, 1},
	{-5, 3},
	{-1, 7},
	{-1, 5},
	{-3, 7},
	{-3, 5},
	{-1, 1},
	{-1, 3},
	{-3, 1},
	{-3, 3},
	{-7,-7},
	{-7,-5},
	{-5,-7},
	{-5,-5},
	{-7,-1},
	{-7,-3},
	{-5,-1},
	{-5,-3},
	{-1,-7},
	{-1,-5},
	{-3,-7},
	{-3,-5},
	{-1,-1},
	{-1,-3},
	{-3,-1},
	{-3,-3}
};
//
// These are the tables used to generate the tx symbols
//
fcmplx a1_qpsk[4];
fcmplx a1_qam16[16];
fcmplx a1_qam64[64];
fcmplx a2_qam16[16];
fcmplx a2_qam64[64];
fcmplx a4_qam16[16];
fcmplx a4_qam64[64];
//
// Look up tables for pilot tones
//
FLOAT pc_stab_cont[2];
FLOAT pc_stab_scat[2];
FLOAT pc_stab_tps[2];

void build_tx_sym_tabs( DVBTFormat *fmt )
{
	int i;
    float tr,ti,f,a;

    // Find the correct average energy to use

    if( fmt->tm == TM_8K )
        a = (float)AVG_E8;
    else
        a = (float)AVG_E2;

    // QPSK factor
    f = 1.0f/sqrtf(2.0);

    for( i = 0; i < 4; i++ )
	{
        a1_qpsk[i].re = c_qpsk[i].re*f*a;
        a1_qpsk[i].im = c_qpsk[i].im*f*a;
	}
    // QAM16 factor
    f = 1.0f/sqrtf(10.0);

	for( i = 0; i < 16; i++ )
	{
        a1_qam16[i].re = c_qam16[i].re*f*a;
        a1_qam16[i].im = c_qam16[i].im*f*a;
	}
    // QAM 64 factor
    f = 1.0f/sqrtf(42.0);

	for( i = 0; i < 64; i++ )
	{
         a1_qam64[i].re = c_qam64[i].re*f*a;
         a1_qam64[i].im = c_qam64[i].im*f*a;
	}
    // QAM16 A2 factor
    f = 1.0f/sqrtf(20.0);

    for( i = 0; i < 16; i++ )
	{
        tr = c_qam16[i].re;
        ti = c_qam16[i].im;
		tr = tr > 0 ? tr+1 : tr-1;
		ti = ti > 0 ? ti+1 : ti-1;

        a2_qam16[i].re = tr*f*a;
        a2_qam16[i].im = ti*f*a;
	}
    // QAM64 A2 factor
    f = 1.0f/sqrtf(60.0);

	for( i = 0; i < 64; i++ )
	{
        tr = c_qam64[i].re;
        ti = c_qam64[i].im;
		tr = tr > 0 ? tr+1 : tr-1;
		ti = ti > 0 ? ti+1 : ti-1;

        a2_qam64[i].re = tr*f*a;
        a2_qam64[i].im = ti*f*a;
	}
    // QAM16 A4 factor
    f = 1.0f/sqrtf(52.0);

	for( i = 0; i < 16; i++ )
	{
        tr = c_qam16[i].re;
        ti = c_qam16[i].im;
		tr = tr > 0 ? tr+3 : tr-3;
		ti = ti > 0 ? ti+3 : ti-3;

        a4_qam16[i].re = tr*f*a;
        a4_qam16[i].im = ti*f*a;
	}
    // QAM64 A4 factor
    f = 1.0f/sqrtf(108.0);

	for( i = 0; i < 64; i++ )
	{
        tr = c_qam64[i].re;
        ti = c_qam64[i].im;
		tr = tr > 0 ? tr+3 : tr-3;
		ti = ti > 0 ? ti+3 : ti-3;

        a4_qam64[i].re = tr*f*a;
        a4_qam64[i].im = ti*f*a;
	}
    // Pilot tones (Fixed)
    f = 4.0f/3.0f;
    pc_stab_cont[0] =   1.0f*f*a;
    pc_stab_cont[1] =  -1.0f*f*a;

    pc_stab_scat[0] =   1.0f*f*a;
    pc_stab_scat[1] =  -1.0f*f*a;
    // TP tones
    f = 1.0;
    pc_stab_tps[0]   =  1.0f*f*a;
    pc_stab_tps[1]   = -1.0f*f*a;

}
