
#include <stdio.h>
#include <math.h>

double p1evl(double, double [], int);
double polevl(double, double [], int);
double igam(double a, double x);
double lgam(double x);
int mtherr(char *name, int code);


int merror = 0;

/* Notice: the order of appearance of the following
 * messages is bound to the error codes defined
 * in mconf.h.
 */
static char *ermsg[7] = {
	"unknown",      /* error code 0 */
	"domain",       /* error code 1 */
	"singularity",  /* et seq.      */
	"overflow",
	"underflow",
	"total loss of precision",
	"partial loss of precision"
};



/*                                                      const.c
 *
 *     Globally declared constants
 *
 *
 *
 * SYNOPSIS:
 *
 * extern double nameofconstant;
 *
 *
 *
 *
 * DESCRIPTION:
 *
 * This file contains a number of mathematical constants and
 * also some needed size parameters of the computer arithmetic.
 * The values are supplied as arrays of hexadecimal integers
 * for IEEE arithmetic; arrays of octal constants for DEC
 * arithmetic; and in a normal decimal scientific notation for
 * other machines.  The particular notation used is determined
 * by a symbol (DEC, IBMPC, or UNK) defined in the include file
 * mconf.h.
 *
 * The default size parameters are as follows.
 *
 * For DEC and UNK modes:
 * MACHEP =  1.38777878078144567553E-17       2**-56
 * MAXLOG =  8.8029691931113054295988E1       log(2**127)
 * MINLOG = -8.872283911167299960540E1        log(2**-128)
 * MAXNUM =  1.701411834604692317316873e38    2**127
 *
 * For IEEE arithmetic (IBMPC):
 * MACHEP =  1.11022302462515654042E-16       2**-53
 * MAXLOG =  7.09782712893383996843E2         log(2**1024)
 * MINLOG = -7.08396418532264106224E2         log(2**-1022)
 * MAXNUM =  1.7976931348623158E308           2**1024
 *
 * The global symbols for mathematical constants are
 * PI     =  3.14159265358979323846           pi
 * PIO2   =  1.57079632679489661923           pi/2
 * PIO4   =  7.85398163397448309616E-1        pi/4
 * SQRT2  =  1.41421356237309504880           sqrt(2)
 * SQRTH  =  7.07106781186547524401E-1        sqrt(2)/2
 * LOG2E  =  1.4426950408889634073599         1/log(2)
 * SQ2OPI =  7.9788456080286535587989E-1      sqrt( 2/pi )
 * LOGE2  =  6.93147180559945309417E-1        log(2)
 * LOGSQ2 =  3.46573590279972654709E-1        log(2)/2
 * THPIO4 =  2.35619449019234492885           3*pi/4
 * TWOOPI =  6.36619772367581343075535E-1     2/pi
 *
 * These lists are subject to change.
 */

/*                                                      const.c */

/*
Cephes Math Library Release 2.3:  March, 1995
Copyright 1984, 1995 by Stephen L. Moshier
*/

double MACHEP =  1.11022302462515654042E-16;   /* 2**-53 */
double UFLOWTHRESH =  2.22507385850720138309E-308; /* 2**-1022 */
#if 1//DENORMAL
double MAXLOG =  7.09782712893383996732E2;     /* log(MAXNUM) */
double MINLOG = -7.451332191019412076235E2;     /* log(2**-1075) */
#else
double MAXLOG =  7.08396418532264106224E2;     /* log 2**1022 */
double MINLOG = -7.08396418532264106224E2;     /* log 2**-1022 */
#endif
double MAXNUM =  1.79769313486231570815E308;    /* 2**1024*(1-MACHEP) */
double PI     =  3.14159265358979323846;       /* pi */
double PIO2   =  1.57079632679489661923;       /* pi/2 */
double PIO4   =  7.85398163397448309616E-1;    /* pi/4 */
double SQRT2  =  1.41421356237309504880;       /* sqrt(2) */
double SQRTH  =  7.07106781186547524401E-1;    /* sqrt(2)/2 */
double LOG2E  =  1.4426950408889634073599;     /* 1/log(2) */
double SQ2OPI =  7.9788456080286535587989E-1;  /* sqrt( 2/pi ) */
double LOGE2  =  6.93147180559945309417E-1;    /* log(2) */
double LOGSQ2 =  3.46573590279972654709E-1;    /* log(2)/2 */
double THPIO4 =  2.35619449019234492885;       /* 3*pi/4 */
double TWOOPI =  6.36619772367581343075535E-1; /* 2/pi */
#ifdef INFINITY

#else
double INFINITY =  1.79769313486231570815E308;    /* 2**1024*(1-MACHEP) */
#endif
#ifdef NAN

#else
double NAN = 0.0;
#endif
double NEGZERO = 0.0;

static double big = 4.503599627370496e15;
static double biginv =  2.22044604925031308085e-16;

static double P[] = {
	1.60119522476751861407E-4,
	1.19135147006586384913E-3,
	1.04213797561761569935E-2,
	4.76367800457137231464E-2,
	2.07448227648435975150E-1,
	4.94214826801497100753E-1,
	9.99999999999999996796E-1
};
static double Q[] = {
	-2.31581873324120129819E-5,
	 5.39605580493303397842E-4,
	-4.45641913851797240494E-3,
	 1.18139785222060435552E-2,
	 3.58236398605498653373E-2,
	-2.34591795718243348568E-1,
	 7.14304917030273074085E-2,
	 1.00000000000000000320E0
};
#define MAXGAM 171.624376956302725
static double LOGPI = 1.14472988584940017414;

/* Stirling's formula for the gamma function */
static double STIR[5] = {
	 7.87311395793093628397E-4,
	-2.29549961613378126380E-4,
	-2.68132617805781232825E-3,
	 3.47222221605458667310E-3,
	 8.33333333333482257126E-2,
};
#define MAXSTIR 143.01608
static double SQTPI = 2.50662827463100050242E0;

int sgngam = 0;
extern int sgngam;
extern double MAXLOG, MAXNUM, PI;

/*                                                      igam.c
 *
 *     Incomplete gamma integral
 *
 *
 *
 * SYNOPSIS:
 *
 * double a, x, y, igam();
 *
 * y = igam( a, x );
 *
 * DESCRIPTION:
 *
 * The function is defined by
 *
 *                           x
 *                            -
 *                   1       | |  -t  a-1
 *  igam(a,x)  =   -----     |   e   t   dt.
 *                  -      | |
 *                 | (a)    -
 *                           0
 *
 *
 * In this implementation both arguments must be positive.
 * The integral is evaluated by either a power series or
 * continued fraction expansion, depending on the relative
 * values of a and x.
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    IEEE      0,30       200000       3.6e-14     2.9e-15
 *    IEEE      0,100      300000       9.9e-14     1.5e-14
 */
/*                                                     igamc()
 *
 *     Complemented incomplete gamma integral
 *
 *
 *
 * SYNOPSIS:
 *
 * double a, x, y, igamc();
 *
 * y = igamc( a, x );
 *
 * DESCRIPTION:
 *
 * The function is defined by
 *
 *
 *  igamc(a,x)   =   1 - igam(a,x)
 *
 *                            inf.
 *                              -
 *                     1       | |  -t  a-1
 *               =   -----     |   e   t   dt.
 *                    -      | |
 *                   | (a)    -
 *                             x
 *
 *
 * In this implementation both arguments must be positive.
 * The integral is evaluated by either a power series or
 * continued fraction expansion, depending on the relative
 * values of a and x.
 *
 * ACCURACY:
 *
 * Tested at random a, x.
 *                a         x                      Relative error:
 * arithmetic   domain   domain     # trials      peak         rms
 *    IEEE     0.5,100   0,100      200000       1.9e-14     1.7e-15
 *    IEEE     0.01,0.5  0,100      200000       1.4e-13     1.6e-15
 */

/*
Cephes Math Library Release 2.0:  April, 1987
Copyright 1985, 1987 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/

double igamc(double a, double x)
{
	double ans, ax, c, yc, r, t, y, z;
	double pk, pkm1, pkm2, qk, qkm1, qkm2;

	if( (x <= 0) || ( a <= 0) )
		return( 1.0 );

	if( (x < 1.0) || (x < a) )
		return( 1.e0 - igam(a,x) );

	ax = a * log(x) - x - lgam(a);
	if( ax < -MAXLOG ) {
		mtherr( "igamc", UNDERFLOW );
		return( 0.0 );
	}
	ax = exp(ax);

	/* continued fraction */
	y = 1.0 - a;
	z = x + y + 1.0;
	c = 0.0;
	pkm2 = 1.0;
	qkm2 = x;
	pkm1 = x + 1.0;
	qkm1 = z * x;
	ans = pkm1/qkm1;

	do {
		c += 1.0;
		y += 1.0;
		z += 2.0;
		yc = y * c;
		pk = pkm1 * z  -  pkm2 * yc;
		qk = qkm1 * z  -  qkm2 * yc;
		if( qk != 0 ) {
			r = pk/qk;
			t = fabs( (ans - r)/r );
			ans = r;
		}
		else
			t = 1.0;
		pkm2 = pkm1;
		pkm1 = pk;
		qkm2 = qkm1;
		qkm1 = qk;
		if( fabs(pk) > big ) {
			pkm2 *= biginv;
			pkm1 *= biginv;
			qkm2 *= biginv;
			qkm1 *= biginv;
		}
	} while( t > MACHEP );

	return( ans * ax );
}



/* left tail of incomplete gamma function:
 *
 *          inf.      k
 *   a  -x   -       x
 *  x  e     >   ----------
 *           -     -
 *          k=0   | (a+k+1)
 *
 */

double igam(double a, double x)
{
	double ans, ax, c, r;

	if( (x <= 0) || ( a <= 0) )
		return( 0.0 );

	if( (x > 1.0) && (x > a ) )
		return( 1.e0 - igamc(a,x) );

	/* Compute  x**a * exp(-x) / gamma(a)  */
	ax = a * log(x) - x - lgam(a);
	if( ax < -MAXLOG ) {
		mtherr( "igam", UNDERFLOW );
		return( 0.0 );
	}
	ax = exp(ax);

	/* power series */
	r = a;
	c = 1.0;
	ans = 1.0;

	do {
		r += 1.0;
		c *= x/r;
		ans += c;
	} while( c/ans > MACHEP );

	return( ans * ax/a );
}

/*                                                      gamma.c
 *
 *     Gamma function
 *
 *
 *
 * SYNOPSIS:
 *
 * double x, y, gamma();
 * extern int sgngam;
 *
 * y = gamma( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Returns gamma function of the argument.  The result is
 * correctly signed, and the sign (+1 or -1) is also
 * returned in a global (extern) variable named sgngam.
 * This variable is also filled in by the logarithmic gamma
 * function lgam().
 *
 * Arguments |x| <= 34 are reduced by recurrence and the function
 * approximated by a rational function of degree 6/7 in the
 * interval (2,3).  Large arguments are handled by Stirling's
 * formula. Large negative arguments are made positive using
 * a reflection formula.
 *
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    DEC      -34, 34      10000       1.3e-16     2.5e-17
 *    IEEE    -170,-33      20000       2.3e-15     3.3e-16
 *    IEEE     -33,  33     20000       9.4e-16     2.2e-16
 *    IEEE      33, 171.6   20000       2.3e-15     3.2e-16
 *
 * Error for arguments outside the test range will be larger
 * owing to error amplification by the exponential function.
 *
 */
/*                                                      lgam()
 *
 *     Natural logarithm of gamma function
 *
 *
 *
 * SYNOPSIS:
 *
 * double x, y, lgam();
 * extern int sgngam;
 *
 * y = lgam( x );
 *
 *
 *
 * DESCRIPTION:
 *
 * Returns the base e (2.718...) logarithm of the absolute
 * value of the gamma function of the argument.
 * The sign (+1 or -1) of the gamma function is returned in a
 * global (extern) variable named sgngam.
 *
 * For arguments greater than 13, the logarithm of the gamma
 * function is approximated by the logarithmic version of
 * Stirling's formula using a polynomial approximation of
 * degree 4. Arguments between -33 and +33 are reduced by
 * recurrence to the interval [2,3] of a rational approximation.
 * The cosecant reflection formula is employed for arguments
 * less than -33.
 *
 * Arguments greater than MAXLGM return MAXNUM and an error
 * message.  MAXLGM = 2.035093e36 for DEC
 * arithmetic or 2.556348e305 for IEEE arithmetic.
 *
 *
 *
 * ACCURACY:
 *
 *
 * arithmetic      domain        # trials     peak         rms
 *    DEC     0, 3                  7000     5.2e-17     1.3e-17
 *    DEC     2.718, 2.035e36       5000     3.9e-17     9.9e-18
 *    IEEE    0, 3                 28000     5.4e-16     1.1e-16
 *    IEEE    2.718, 2.556e305     40000     3.5e-16     8.3e-17
 * The error criterion was relative when the function magnitude
 * was greater than one but absolute when it was less than one.
 *
 * The following test used the relative error criterion, though
 * at certain points the relative error could be much higher than
 * indicated.
 *    IEEE    -200, -4             10000     4.8e-16     1.3e-16
 *
 */

/*                                                      gamma.c */
/*      gamma function  */

/*
Cephes Math Library Release 2.2:  July, 1992
Copyright 1984, 1987, 1989, 1992 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/

/* Gamma function computed by Stirling's formula.
 * The polynomial STIR is valid for 33 <= x <= 172.
 */
static double stirf(double x)
{
	double y, w, v;

	w = 1.0/x;
	w = 1.0 + w * polevl( w, STIR, 4 );
	y = exp(x);
	if( x > MAXSTIR ) { /* Avoid overflow in pow() */
		v = pow( x, 0.5 * x - 0.25 );
		y = v * (v / y);
	}
	else
		y = pow( x, x - 0.5 ) / y;
	y = SQTPI * y * w;

	return( y );
}



double gamma(double x)
{
	double p, q, z;
	int i;

	sgngam = 1;

	q = fabs(x);

	if( q > 33.0 ) {
		if( x < 0.0 ) {
			p = floor(q);
			if( p == q ) {
				goto goverf;
			}
			i = (int)p;
			if( (i & 1) == 0 )
				sgngam = -1;
			z = q - p;
			if( z > 0.5 ) {
				p += 1.0;
				z = q - p;
			}
			z = q * sin( PI * z );
			if( z == 0.0 ) {
				goverf:
				mtherr( "gamma", OVERFLOW );
				return( sgngam * MAXNUM);
			}
			z = fabs(z);
			z = PI/(z * stirf(q) );
		}
	else {
		z = stirf(x);
	}

	return( sgngam * z );
	}

	z = 1.0;
	while( x >= 3.0 ) {
		x -= 1.0;
		z *= x;
	}

	while( x < 0.0 ) {
		if( x > -1.E-9 )
			goto small;
		z /= x;
		x += 1.0;
	}

	while( x < 2.0 ) {
		if( x < 1.e-9 )
			goto small;
		z /= x;
		x += 1.0;
	}

	if( x == 2.0 )
		return(z);

	x -= 2.0;
	p = polevl( x, P, 6 );
	q = polevl( x, Q, 7 );
	return( z * p / q );

small:
	if( x == 0.0 ) {
		mtherr( "gamma", SING );
		return( MAXNUM );
	}
else

	return( z/((1.0 + 0.5772156649015329 * x) * x) );
}



/* A[]: Stirling's formula expansion of log gamma
 * B[], C[]: log gamma function between 2 and 3
 */
static double A[] = {
	 8.11614167470508450300E-4,
	-5.95061904284301438324E-4,
	 7.93650340457716943945E-4,
	-2.77777777730099687205E-3,
	 8.33333333333331927722E-2
};
static double B[] = {
	-1.37825152569120859100E3,
	-3.88016315134637840924E4,
	-3.31612992738871184744E5,
	-1.16237097492762307383E6,
	-1.72173700820839662146E6,
	-8.53555664245765465627E5
};
static double C[] = {
	/* 1.00000000000000000000E0, */
	-3.51815701436523470549E2,
	-1.70642106651881159223E4,
	-2.20528590553854454839E5,
	-1.13933444367982507207E6,
	-2.53252307177582951285E6,
	-2.01889141433532773231E6
};
/* log( sqrt( 2*pi ) ) */
static double LS2PI  =  0.91893853320467274178;
#define MAXLGM 2.556348e305


/* Logarithm of gamma function */


double lgam(double x)
{
	double p, q, u, w, z;
	int i;

	sgngam = 1;

	if( x < -34.0 ) {
		q = -x;
		w = lgam(q); /* note this modifies sgngam! */
		p = floor(q);
		if( p == q ) {
			lgsing:
			goto loverf;
		}
		i = (int)p;
		if( (i & 1) == 0 )
			sgngam = -1;
		else
			sgngam = 1;
		z = q - p;
		if( z > 0.5 ) {
			p += 1.0;
			z = p - q;
		}
		z = q * sin( PI * z );
		if( z == 0.0 )
			goto lgsing;
		/*      z = log(PI) - log( z ) - w;*/
		z = LOGPI - log( z ) - w;
		return( z );
	}

	if( x < 13.0 ) {
		z = 1.0;
		p = 0.0;
		u = x;
		while( u >= 3.0 ) {
			p -= 1.0;
			u = x + p;
			z *= u;
		}
		while( u < 2.0 ) {
			if( u == 0.0 )
				goto lgsing;
			z /= u;
			p += 1.0;
			u = x + p;
		}
		if( z < 0.0 ) {
			sgngam = -1;
			z = -z;
		}
		else
			sgngam = 1;
		if( u == 2.0 )
			return( log(z) );
		p -= 2.0;
		x = x + p;
		p = x * polevl( x, B, 5 ) / p1evl( x, C, 6);
		return( log(z) + p );
	}

	if( x > MAXLGM ) {
loverf:
		mtherr( "lgam", OVERFLOW );
		return( sgngam * MAXNUM );
	}

	q = ( x - 0.5 ) * log(x) - x + LS2PI;
	if( x > 1.0e8 )
	return( q );

	p = 1.0/(x*x);
	if( x >= 1000.0 )
		q += ((   7.9365079365079365079365e-4 * p
			 - 2.7777777777777777777778e-3) *p
			 + 0.0833333333333333333333) / x;
	else
		q += polevl( p, A, 4 ) / x;

	return( q );
}

/*                                                      mtherr.c
 *
 *     Library common error handling routine
 *
 *
 *
 * SYNOPSIS:
 *
 * char *fctnam;
 * int code;
 * int mtherr();
 *
 * mtherr( fctnam, code );
 *
 *
 *
 * DESCRIPTION:
 *
 * This routine may be called to report one of the following
 * error conditions (in the include file mconf.h).
 *
 *   Mnemonic        Value          Significance
 *
 *    DOMAIN            1       argument domain error
 *    SING              2       function singularity
 *    OVERFLOW          3       overflow range error
 *    UNDERFLOW         4       underflow range error
 *    TLOSS             5       total loss of precision
 *    PLOSS             6       partial loss of precision
 *    EDOM             33       Unix domain error code
 *    ERANGE           34       Unix range error code
 *
 * The default version of the file prints the function name,
 * passed to it by the pointer fctnam, followed by the
 * error condition.  The display is directed to the standard
 * output device.  The routine then returns to the calling
 * program.  Users may wish to modify the program to abort by
 * calling exit() under severe error conditions such as domain
 * errors.
 *
 * Since all error conditions pass control to this function,
 * the display may be easily changed, eliminated, or directed
 * to an error logging device.
 *
 * SEE ALSO:
 *
 * mconf.h
 *
 */

/*
Cephes Math Library Release 2.0:  April, 1987
Copyright 1984, 1987 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/

int mtherr(char *name, int code)
{

	/* Display string passed by calling program,
	 * which is supposed to be the name of the
	 * function in which the error occurred:
	 */
	/* Set global error message word */
	merror = code;

	/* Display error message defined
	 * by the code argument.
	 */
	if( (code <= 0) || (code >= 7) )
		code = 0;

	return( 0 );
}

/*                                                      polevl.c
 *                                                     p1evl.c
 *
 *     Evaluate polynomial
 *
 *
 *
 * SYNOPSIS:
 *
 * int N;
 * double x, y, coef[N+1], polevl[];
 *
 * y = polevl( x, coef, N );
 *
 *
 *
 * DESCRIPTION:
 *
 * Evaluates polynomial of degree N:
 *
 *                     2          N
 * y  =  C  + C x + C x  +...+ C x
 *        0    1     2          N
 *
 * Coefficients are stored in reverse order:
 *
 * coef[0] = C  , ..., coef[N] = C  .
 *            N                   0
 *
 *  The function p1evl() assumes that coef[N] = 1.0 and is
 * omitted from the array.  Its calling arguments are
 * otherwise the same as polevl().
 *
 *
 * SPEED:
 *
 * In the interest of speed, there are no checks for out
 * of bounds arithmetic.  This routine is used by most of
 * the functions in the library.  Depending on available
 * equipment features, the user may wish to rewrite the
 * program in microcode or assembly language.
 *
 */


/*
Cephes Math Library Release 2.1:  December, 1988
Copyright 1984, 1987, 1988 by Stephen L. Moshier
Direct inquiries to 30 Frost Street, Cambridge, MA 02140
*/

double polevl(double x, double coef[], int N)
{
	double	ans;
	int		i;
	double	*p;

	p = coef;
	ans = *p++;
	i = N;

	do {
		ans = ans * x  +  *p++;
	} while( --i );

	return(ans);
}

/*                                                      p1evl() */
/*                                          N
 * Evaluate polynomial when coefficient of x  is 1.0.
 * Otherwise same as polevl.
 */

double p1evl(double x, double coef[], int N)
{
	double	ans;
	double	*p;
	int		i;

	p = coef;
	ans = x + *p++;
	i = N-1;

	do {
		ans = ans * x  + *p++;
	} while( --i );

	return(ans);
}


/* error function in double precision */

double erf_ext(double x)
{
    int k;
    double w, t, y;
    static double a[65] = {
        5.958930743e-11, -1.13739022964e-9, 
        1.466005199839e-8, -1.635035446196e-7, 
        1.6461004480962e-6, -1.492559551950604e-5, 
        1.2055331122299265e-4, -8.548326981129666e-4, 
        0.00522397762482322257, -0.0268661706450773342, 
        0.11283791670954881569, -0.37612638903183748117, 
        1.12837916709551257377, 
        2.372510631e-11, -4.5493253732e-10, 
        5.90362766598e-9, -6.642090827576e-8, 
        6.7595634268133e-7, -6.21188515924e-6, 
        5.10388300970969e-5, -3.7015410692956173e-4, 
        0.00233307631218880978, -0.0125498847718219221, 
        0.05657061146827041994, -0.2137966477645600658, 
        0.84270079294971486929, 
        9.49905026e-12, -1.8310229805e-10, 
        2.39463074e-9, -2.721444369609e-8, 
        2.8045522331686e-7, -2.61830022482897e-6, 
        2.195455056768781e-5, -1.6358986921372656e-4, 
        0.00107052153564110318, -0.00608284718113590151, 
        0.02986978465246258244, -0.13055593046562267625, 
        0.67493323603965504676, 
        3.82722073e-12, -7.421598602e-11, 
        9.793057408e-10, -1.126008898854e-8, 
        1.1775134830784e-7, -1.1199275838265e-6, 
        9.62023443095201e-6, -7.404402135070773e-5, 
        5.0689993654144881e-4, -0.00307553051439272889, 
        0.01668977892553165586, -0.08548534594781312114, 
        0.56909076642393639985, 
        1.55296588e-12, -3.032205868e-11, 
        4.0424830707e-10, -4.71135111493e-9, 
        5.011915876293e-8, -4.8722516178974e-7, 
        4.30683284629395e-6, -3.445026145385764e-5, 
        2.4879276133931664e-4, -0.00162940941748079288, 
        0.00988786373932350462, -0.05962426839442303805, 
        0.49766113250947636708
    };
    static double b[65] = {
        -2.9734388465e-10, 2.69776334046e-9, 
        -6.40788827665e-9, -1.6678201321e-8, 
        -2.1854388148686e-7, 2.66246030457984e-6, 
        1.612722157047886e-5, -2.5616361025506629e-4, 
        1.5380842432375365e-4, 0.00815533022524927908, 
        -0.01402283663896319337, -0.19746892495383021487, 
        0.71511720328842845913, 
        -1.951073787e-11, -3.2302692214e-10, 
        5.22461866919e-9, 3.42940918551e-9, 
        -3.5772874310272e-7, 1.9999935792654e-7, 
        2.687044575042908e-5, -1.1843240273775776e-4, 
        -8.0991728956032271e-4, 0.00661062970502241174, 
        0.00909530922354827295, -0.2016007277849101314, 
        0.51169696718727644908, 
        3.147682272e-11, -4.8465972408e-10, 
        6.3675740242e-10, 3.377623323271e-8, 
        -1.5451139637086e-7, -2.03340624738438e-6, 
        1.947204525295057e-5, 2.854147231653228e-5, 
        -0.00101565063152200272, 0.00271187003520095655, 
        0.02328095035422810727, -0.16725021123116877197, 
        0.32490054966649436974, 
        2.31936337e-11, -6.303206648e-11, 
        -2.64888267434e-9, 2.050708040581e-8, 
        1.1371857327578e-7, -2.11211337219663e-6, 
        3.68797328322935e-6, 9.823686253424796e-5, 
        -6.5860243990455368e-4, -7.5285814895230877e-4, 
        0.02585434424202960464, -0.11637092784486193258, 
        0.18267336775296612024, 
        -3.67789363e-12, 2.0876046746e-10, 
        -1.93319027226e-9, -4.35953392472e-9, 
        1.8006992266137e-7, -7.8441223763969e-7, 
        -6.75407647949153e-6, 8.428418334440096e-5, 
        -1.7604388937031815e-4, -0.0023972961143507161, 
        0.0206412902387602297, -0.06905562880005864105, 
        0.09084526782065478489
    };

    w = x < 0 ? -x : x;
    if (w < 2.2) {
        t = w * w;
        k = (int) t;
        t -= k;
        k *= 13;
        y = ((((((((((((a[k] * t + a[k + 1]) * t + 
            a[k + 2]) * t + a[k + 3]) * t + a[k + 4]) * t + 
            a[k + 5]) * t + a[k + 6]) * t + a[k + 7]) * t + 
            a[k + 8]) * t + a[k + 9]) * t + a[k + 10]) * t + 
            a[k + 11]) * t + a[k + 12]) * w;
    } else if (w < 6.9) {
        k = (int) w;
        t = w - k;
        k = 13 * (k - 2);
        y = (((((((((((b[k] * t + b[k + 1]) * t + 
            b[k + 2]) * t + b[k + 3]) * t + b[k + 4]) * t + 
            b[k + 5]) * t + b[k + 6]) * t + b[k + 7]) * t + 
            b[k + 8]) * t + b[k + 9]) * t + b[k + 10]) * t + 
            b[k + 11]) * t + b[k + 12];
        y *= y;
        y *= y;
        y *= y;
        y = 1 - y * y;
    } else {
        y = 1;
    }
    return x < 0 ? -y : y;
}


/* error function in double precision */

double erfc_ext(double x)
{
    double t, u, y;

    t = 3.97886080735226 / (fabs(x) + 3.97886080735226);
    u = t - 0.5;
    y = (((((((((0.00127109764952614092 * u + 1.19314022838340944e-4) * u - 
        0.003963850973605135) * u - 8.70779635317295828e-4) * u + 
        0.00773672528313526668) * u + 0.00383335126264887303) * u - 
        0.0127223813782122755) * u - 0.0133823644533460069) * u + 
        0.0161315329733252248) * u + 0.0390976845588484035) * u + 
        0.00249367200053503304;
    y = ((((((((((((y * u - 0.0838864557023001992) * u - 
        0.119463959964325415) * u + 0.0166207924969367356) * u + 
        0.357524274449531043) * u + 0.805276408752910567) * u + 
        1.18902982909273333) * u + 1.37040217682338167) * u + 
        1.31314653831023098) * u + 1.07925515155856677) * u + 
        0.774368199119538609) * u + 0.490165080585318424) * u + 
        0.275374741597376782) * t * exp(-x * x);
    return x < 0 ? 2 - y : y;
}
