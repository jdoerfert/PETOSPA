#include "rsbench.h"

// This function uses a combination of the Abrarov Approximation
// and the QUICK_W three term asymptotic expansion.
// Only expected to use Abrarov ~0.5% of the time.
double complex fast_nuclear_W( double complex Z )
{
	// Abrarov 
	if( cabs(Z) < 6.0 )
	{
		// Precomputed parts for speeding things up
		// (N = 10, Tm = 12.0)
		double complex prefactor = 8.124330e+01 * I;
		double an[10] = {
			2.758402e-01,
			2.245740e-01,
			1.594149e-01,
			9.866577e-02,
			5.324414e-02,
			2.505215e-02,
			1.027747e-02,
			3.676164e-03,
			1.146494e-03,
			3.117570e-04
		};
		double neg_1n[10] = {
			-1.0,
			1.0,
			-1.0,
			1.0,
			-1.0,
			1.0,
			-1.0,
			1.0,
			-1.0,
			1.0
		};

		double denominator_left[10] = {
			9.869604e+00,
			3.947842e+01,
			8.882644e+01,
			1.579137e+02,
			2.467401e+02,
			3.553058e+02,
			4.836106e+02,
			6.316547e+02,
			7.994380e+02,
			9.869604e+02
		};

		double complex W = I * ( 1 - fast_cexp(I*12.*Z) ) / (12. * Z );
		double complex sum = 0;
		for( int n = 0; n < 10; n++ )
		{
			complex double top = neg_1n[n] * fast_cexp(I*12.*Z) - 1.;
			complex double bot = denominator_left[n] - 144.*Z*Z;
			sum += an[n] * (top/bot);
		}
		W += prefactor * Z  * sum;
		return W;
	}

	// QUICK_2 3 Term Asymptotic Expansion (Accurate to O(1e-6)).
	// Pre-computed parameters
	double a = 0.512424224754768462984202823134979415014943561548661637413182;
	double b = 0.275255128608410950901357962647054304017026259671664935783653;
	double c = 0.051765358792987823963876628425793170829107067780337219430904;
	double d = 2.724744871391589049098642037352945695982973740328335064216346;

	// Three Term Asymptotic Expansion
	double complex W = I * Z * (a/(Z*Z - b) + c/(Z*Z - d));

	return W;
}

void calculate_macro_xs( double * macro_xs, int mat, double E, Input input, CalcDataPtrs data, complex double * sigTfactors, long * abrarov, long * alls ) 
{
	// zero out macro vector
	for( int i = 0; i < 4; i++ )
		macro_xs[i] = 0;

	// for nuclide in mat
	for( int i = 0; i < (data.materials).num_nucs[mat]; i++ )
	{
		double micro_xs[4];
		int nuc = (data.materials).mats[mat][i];

		if( input.doppler == 1 )
			calculate_micro_xs_doppler( micro_xs, nuc, E, input, data, sigTfactors, abrarov, alls);
		else
			calculate_micro_xs( micro_xs, nuc, E, input, data, sigTfactors);

		for( int j = 0; j < 4; j++ )
		{
			macro_xs[j] += micro_xs[j] * data.materials.concs[mat][i];
		}
	}

	/* Debug
	printf("E = %.2lf, mat = %d, macro_xs[0] = %.2lf, macro_xs[1] = %.2lf, macro_xs[2] = %.2lf, macro_xs[3] = %.2lf\n",
	E, mat, macro_xs[0], macro_xs[1], macro_xs[2], macro_xs[3] );
	*/
	
}

// No Temperature dependence (i.e., 0K evaluation)
void calculate_micro_xs( double * micro_xs, int nuc, double E, Input input, CalcDataPtrs data, complex double * sigTfactors)
{
	// MicroScopic XS's to Calculate
	double sigT;
	double sigA;
	double sigF;
	double sigE;

	// Calculate Window Index
	double spacing = 1.0 / data.n_windows[nuc];
	int window = (int) ( E / spacing );
	if( window == data.n_windows[nuc] )
		window--;

	// Calculate sigTfactors
	calculate_sig_T(nuc, E, input, data, sigTfactors );

	// Calculate contributions from window "background" (i.e., poles outside window (pre-calculated)
	Window w = data.windows[nuc][window];
	sigT = E * w.T;
	sigA = E * w.A;
	sigF = E * w.F;

	// Loop over Poles within window, add contributions
	for( int i = w.start; i < w.end; i++ )
	{
		complex double PSIIKI;
		complex double CDUM;
		Pole pole = data.poles[nuc][i];
		PSIIKI = -(0.0 - 1.0 * _Complex_I ) / ( pole.MP_EA - sqrt(E) );
		CDUM = PSIIKI / E;
		sigT += creal( pole.MP_RT * CDUM * sigTfactors[pole.l_value] );
		sigA += creal( pole.MP_RA * CDUM);
		sigF += creal( pole.MP_RF * CDUM);
	}

	sigE = sigT - sigA;

	micro_xs[0] = sigT;
	micro_xs[1] = sigA;
	micro_xs[2] = sigF;
	micro_xs[3] = sigE;
}

// Temperature Dependent Variation of Kernel
// (This involves using the Complex Faddeeva function to
// Doppler broaden the poles within the window)
void calculate_micro_xs_doppler( double * micro_xs, int nuc, double E, Input input, CalcDataPtrs data, complex double * sigTfactors, long * abrarov, long * alls)
{
	// MicroScopic XS's to Calculate
	double sigT;
	double sigA;
	double sigF;
	double sigE;

	// Calculate Window Index
	double spacing = 1.0 / data.n_windows[nuc];
	int window = (int) ( E / spacing );
	if( window == data.n_windows[nuc] )
		window--;

	// Calculate sigTfactors
	calculate_sig_T(nuc, E, input, data, sigTfactors );

	// Calculate contributions from window "background" (i.e., poles outside window (pre-calculated)
	Window w = data.windows[nuc][window];
	sigT = E * w.T;
	sigA = E * w.A;
	sigF = E * w.F;

	double dopp = 0.5;

	// Loop over Poles within window, add contributions
	for( int i = w.start; i < w.end; i++ )
	{
		Pole pole = data.poles[nuc][i];

		// Prep Z
		double complex Z = (E - pole.MP_EA) * dopp;
		if( cabs(Z) < 6.0 )
			(*abrarov)++;
		(*alls)++;

		// Evaluate Fadeeva Function
		complex double faddeeva = fast_nuclear_W( Z );

		// Update W
		sigT += creal( pole.MP_RT * faddeeva * sigTfactors[pole.l_value] );
		sigA += creal( pole.MP_RA * faddeeva);
		sigF += creal( pole.MP_RF * faddeeva);
	}

	sigE = sigT - sigA;

	micro_xs[0] = sigT;
	micro_xs[1] = sigA;
	micro_xs[2] = sigF;
	micro_xs[3] = sigE;
}

void calculate_sig_T( int nuc, double E, Input input, CalcDataPtrs data, complex double * sigTfactors )
{
	double phi;

	for( int i = 0; i < input.numL; i++ )
	{
		phi = data.pseudo_K0RS[nuc][i] * sqrt(E);

		if( i == 1 )
			phi -= - atan( phi );
		else if( i == 2 )
			phi -= atan( 3.0 * phi / (3.0 - phi*phi));
		else if( i == 3 )
			phi -= atan(phi*(15.0-phi*phi)/(15.0-6.0*phi*phi));

		phi *= 2.0;

		sigTfactors[i] = cos(phi) - sin(phi) * _Complex_I;
	}
}
static const char *calculate_macro_xs_OptimisticChoices = "#c1111111#c51111#c<222222#c=222222222#c>2222#c?2222222222222222222222220#c@222222220#cA1#cD3333#cB00001111111111111000011#c811#c91";
const char **KeepAlive_calculate_macro_xs_xs_kernel = &calculate_macro_xs_OptimisticChoices;
static const char *calculate_micro_xs_doppler_OptimisticChoices = "#c1111111#c51111#c:1#c<222222#c=22222#c>2222#c?22222222222222222#c@22222#cB11111111000#cD3333#cE7";
const char **KeepAlive_calculate_micro_xs_doppler_xs_kernel = &calculate_micro_xs_doppler_OptimisticChoices;
static const char *calculate_micro_xs_OptimisticChoices = "#c11111#c511#c:1#c<2222#c=22222#c>22#c?2222222222222#c@22222#cB111111111#cD33#cE0";
const char **KeepAlive_calculate_micro_xs_xs_kernel = &calculate_micro_xs_OptimisticChoices;
static const char *calculate_sig_T_OptimisticChoices = "#c1111#c51#c:1#c<222#c=22#c>2#c?222222#c@22#cD3#cE7#cB11";
const char **KeepAlive_calculate_sig_T_xs_kernel = &calculate_sig_T_OptimisticChoices;
static const char *fast_nuclear_W_OptimisticChoices = "#c51#c:1#c?222#cE=";
const char **KeepAlive_fast_nuclear_W_xs_kernel = &fast_nuclear_W_OptimisticChoices;
static const char *fast_cexp_OptimisticChoices = "#c71#cE=";
const char **KeepAlive_fast_cexp_xs_kernel = &fast_cexp_OptimisticChoices;
static const char *__muldc3_OptimisticChoices = "#c71#cE0";
const char **KeepAlive___muldc3_xs_kernel = &__muldc3_OptimisticChoices;
static const char *llvm_assume_OptimisticChoices = "#cE7";
const char **KeepAlive_llvm_assume_xs_kernel = &llvm_assume_OptimisticChoices;
