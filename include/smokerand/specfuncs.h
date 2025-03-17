/**
 * @file specfuncs.h
 * @brief Some special functions required for computation of p-values for
 * different statistical criteria.
 * @details This file is designed for C89 (ANSI C)! Don't use features
 * from C99 and later!
 * @copyright (c) 2024 Alexey L. Voskov, Lomonosov Moscow State University.
 * alvoskov@gmail.com
 *
 * This software is licensed under the MIT license.
 */
#ifndef __SMOKERAND_SPECFUNCS_H
#define __SMOKERAND_SPECFUNCS_H
double sr_expm1(double x);
double sr_log2(double x);
double sr_lgamma(double x);
double sr_betainc(double x, double a, double b, double *cIx_out);
double sr_round(double x);
double sr_ks_pvalue(double x);
double sr_gammainc(double a, double x);
double sr_binomial_pdf(unsigned long k, unsigned long n, double p);
double sr_binomial_cdf(unsigned long k, unsigned long n, double p);
double sr_binomial_pvalue(double k, double n, double p);
double sr_poisson_cdf(double x, double lambda);
double sr_poisson_pvalue(double x, double lambda);
double sr_stdnorm_cdf(double x);
double sr_stdnorm_pvalue(double x);
double sr_stdnorm_inv(double p);
double sr_halfnormal_pvalue(double x);
double sr_t_cdf(double x, unsigned long f);
double sr_t_pvalue(double x, unsigned long f);
double sr_chi2_cdf(double x, unsigned long f);
double sr_chi2_pvalue(double x, unsigned long f);
double sr_chi2_to_stdnorm_approx(double x, unsigned long f);
double sr_linearcomp_Tcdf(double k);
double sr_linearcomp_Tccdf(double k);
#endif
