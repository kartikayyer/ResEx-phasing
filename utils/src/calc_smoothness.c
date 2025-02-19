#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <gsl/gsl_sf.h>
#include <gsl/gsl_math.h>
#include <omp.h>
#include <complex.h>
#include <fftw3.h>
#include "../../src/utils.h"

int main(int argc, char *argv[]) {
	long size, i, j, n, vol, area, c ;
	float *intens, *slices ;
	int num_bins, *radius, *angle, rad, ang ;
	double binsize, *angcount, *angavg ;
	double *cumulant, threshold, *cutoff ;
	fftwf_complex *fdensity, *rdensity ;
	fftwf_plan inverse ;
	char fname[999] ;
	FILE *fp ;
	
	if (argc < 3) {
		fprintf(stderr, "Calc Smoothness: Effective tangential smoothness of merge as a function of radius\n") ;
		fprintf(stderr, "---------------------------------------------------------------------------------\n") ;
		fprintf(stderr, "Maybe useful to ascertain convergence in terms of noise\n") ;
		fprintf(stderr, "Compare to expected behaviour for a certain speckle size\n") ;
		fprintf(stderr, "\nUsage: %s <intens_fname> <radial_binsize>\n", argv[0]) ;
		fprintf(stderr, "Optional: <output_fname>\n") ;
		fprintf(stderr, "\nOutput: <intens_fname>-smoothness.dat (if <output_fname> not given)\n") ;
		return 1 ;
	}
	size = get_size(argv[1], sizeof(float)) ;
	binsize = atof(argv[2]) ;
	if (argc > 3)
		strcpy(fname, argv[3]) ;
	else
		sprintf(fname, "%s-smoothness.dat", remove_ext(argv[1])) ;
	vol = size*size*size ;
	area = size*size ;
	c = size / 2 ;
	num_bins = size / binsize ;
	
	intens = malloc(vol * sizeof(float)) ;
	slices = malloc(3 * area * sizeof(float)) ;
	radius = malloc(area * sizeof(int)) ;
	angle = malloc(area * sizeof(int)) ;
	angcount = calloc(num_bins * 720, sizeof(double)) ;
	angavg = calloc(3 * num_bins * 720, sizeof(double)) ;
	cumulant = malloc((720 + 1) * sizeof(double)) ;
	cutoff = calloc(num_bins, sizeof(double)) ;
	rdensity = fftwf_malloc(720 * sizeof(fftwf_complex)) ;
	fdensity = fftwf_malloc(720 * sizeof(fftwf_complex)) ;
	inverse = fftwf_plan_dft_1d(720, fdensity, rdensity, FFTW_BACKWARD, FFTW_ESTIMATE) ;
	
	// Read in model
	fp = fopen(argv[1], "rb") ;
	fread(intens, sizeof(float), vol, fp) ;
	fclose(fp) ;
	
	for (i = 0 ; i < vol ; ++i)
	if (intens[i] < -500.)
		intens[i] = 0. ;
	
	for (i = 0 ; i < size ; ++i)
	for (j = 0 ; j < size ; ++j) {
		// Calculate slices
		slices[0*size*size + i*size + j] = intens[c*size*size + i*size + j] ;
		slices[1*size*size + i*size + j] = intens[j*size*size + c*size + i] ;
		slices[2*size*size + i*size + j] = intens[i*size*size + j*size + c] ;
		
		// Calculate radial and angular bins 
		rad = (sqrt((i-c)*(i-c) + (j-c)*(j-c)) / binsize) ;
		radius[i*size + j] = rad ;
		
		ang = round((atan2(j-c, i-c) + M_PI) * 720 / (2.*M_PI)) ;
		if (ang == 720)
			ang = 0 ;
		angle[i*size + j] = ang ;
		
		// Calculate angular average for each slice
		angcount[rad*720 + ang]++ ;
		for (n = 0 ; n < 3 ; ++n)
			angavg[n*num_bins*720 + rad*720 + ang] += slices[n*size*size + i*size + j] ;
	}
	
	for (i = 0 ; i < num_bins*720 ; ++i)
	if (angcount[i] > 0) {
		angavg[0*num_bins*720 + i] /= angcount[i] ;
		angavg[1*num_bins*720 + i] /= angcount[i] ;
		angavg[2*num_bins*720 + i] /= angcount[i] ;
	}
	
	// For each slice
	// For each radial bin
	for (n = 0 ; n < 3 ; ++n)
	for (rad = 0 ; rad < num_bins ; ++rad) {
		// FFT average
		for (ang = 0 ; ang < 720 ; ++ang)
			fdensity[ang] = angavg[n*num_bins*720 + rad*720 + ang] ;
		fftwf_execute(inverse) ;
		
		// Accumulate shifted FFT
		cumulant[0] = 0. ;
		rdensity[0] = 0. ;
		for (ang = 1 ; ang <= 720 ; ++ang)
			cumulant[ang] = cumulant[ang-1] + cabs(rdensity[(ang + (720/2)-1)%720]) ;
		  
		// Get point where value is 10% of maximum
		threshold = 0.1 * cumulant[720] ;
		ang = 0 ;
		while (cumulant[ang] < threshold)
			ang++ ;
		// Average over all slices
		cutoff[rad] += 1. / 3. * (ang+1) / 720 ;
	}
	
	fprintf(stderr, "Writing output to %s\n", fname) ;
	fp = fopen(fname, "w") ;
	fprintf(fp, "Radius Cutoff\n") ;
	for (i = 0 ; i < num_bins; ++i)
		fprintf(fp, "%6.2f %.6e\n", binsize*i, cutoff[i]) ; 
	fclose(fp) ;
	
	free(intens) ;
	free(slices) ;
	free(radius) ;
	free(angle) ;
	free(angcount) ;
	free(angavg) ;
	free(cumulant) ;
	free(cutoff) ;
	fftwf_destroy_plan(inverse) ;
	fftwf_free(rdensity) ;
	fftwf_free(fdensity) ;
	
	return 0 ;
}
