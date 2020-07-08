#ifndef _EEMD_H_
#define _EEMD_H_

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_poly.h>

void emd(double*  input_buf, int NN,double*  output_buf, int MM,
          unsigned int s_num, unsigned int sifts_num);
#endif // _EEMD_H_
