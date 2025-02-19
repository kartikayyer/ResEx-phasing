#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <math.h>
#include <locale.h>
#include "../../src/utils.h"

int main(int argc, char *argv[]) {
	long x, y, z, size, c, vol, num_vox = 0 ;
	double dot = 0., normsq = 0., rsq, scale, rmin, rmax ;
	float *obs_mag, *model_mag ;
	FILE *fp ;
	
	if (argc < 5) {
		fprintf(stderr, "Calc Scale: Calculates scale factor between two intensities\n") ;
		fprintf(stderr, "-----------------------------------------------------------\n") ;
		fprintf(stderr, "Compares two intensity volumes in a given radius range\n") ;
		fprintf(stderr, "\nUsage: %s <sym_model_fname> <merge_fname> <rmin> <rmax>\n", argv[0]) ;
		return 1 ;
	}
	size = get_size(argv[1], sizeof(float)) ;
	rmin = atof(argv[3]) ;
	rmax = atof(argv[4]) ;
	
	setlocale(LC_ALL, "C") ; // For commas in large integers
	c = size / 2 ;
	vol = size*size*size ;
	
	// Parse complex model
	model_mag = malloc(vol * sizeof(float)) ;
	fp = fopen(argv[1], "rb") ;
	fread(model_mag, sizeof(float), vol, fp) ;
	fclose(fp) ;
	
	// Calculate magnitude
	for (x = 0 ; x < vol ; ++x)
		model_mag[x] = sqrt(model_mag[x]) ;
	
	// Parse experimental merge
	obs_mag = malloc(vol * sizeof(float)) ;
	fp = fopen(argv[2], "rb") ;
	fread(obs_mag, sizeof(float), vol, fp) ;
	fclose(fp) ;
	
	// Calculate magnitude
	for (x = 0 ; x < vol ; ++x)
	if (obs_mag[x] > 0.)
		obs_mag[x] = sqrt(obs_mag[x]) ;
	
	// Calculate scale factor
	for (x = 0 ; x < size ; ++x)
	for (y = 0 ; y < size ; ++y)
	for (z = 0 ; z < size ; ++z) {
		rsq = (x-c)*(x-c) + (y-c)*(y-c) + (z-c)*(z-c) ;
		
		if (rsq > rmin*rmin && rsq < rmax*rmax && obs_mag[x*size*size + y*size + z] > 0.) {
			//dot += model_mag[x*size*size + y*size + z] ;
			//normsq += obs_mag[x*size*size + y*size + z] ;
			dot += obs_mag[x*size*size + y*size + z] * model_mag[x*size*size + y*size + z] ;
			normsq += pow(obs_mag[x*size*size + y*size + z], 2.) ;
			num_vox++ ;
		}
	}
	
	scale = dot / normsq ;
	
	printf("Scale factor for magnitude: %f\n", scale) ;
	printf("%'ld voxels contributed to this calculation\n", num_vox) ;
	
	// Free memory
	free(model_mag) ;
	free(obs_mag) ;
	
	return 0 ;
}
