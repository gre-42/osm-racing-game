/*  svd.c -- Singular value decomposition. Translated to 'C' from the
 *           original Algol code in "Handbook for Automatic Computation,
 *           vol. II, Linear Algebra", Springer-Verlag.
 *
 *  (C) 2000, C. Bond. All rights reserved.
 *
 *  This is almost an exact translation from the original, except that
 *  an iteration counter is added to prevent stalls. This corresponds
 *  to similar changes in other translations.
 *
 *  Returns an error code = 0, if no errors and 'k' if a failure to
 *  converge at the 'kth' singular value.
 *
 */
#include "Svd4.hpp"
#include <Mlib/Math/Sort_Svd.hpp>
#include <Mlib/Stats/Min_Max.hpp>

#include <malloc.h> /* for array allocation */
#include <math.h>    /* for 'fabs'           */

using namespace Mlib;

static int svd_hac(int m,int n,int withu,int withv,double eps,double tol,
	const Array<double>& a, Array<double>& q, Array<double>& u, Array<double>& v)
{
	int i,j,k,l,l1,iter,retval;
	double c,f,g,h,s,x,y,z;
	double *e;
	l = -1; // get rid of compiler-warning "l might be used uninitialized"

	e = (double *)calloc(n,sizeof(double));
	retval = 0;

/* Copy 'a' to 'u' */
	for (i=0;i<m;i++) {
		for (j=0;j<n;j++)
			u(i, j) = a(i, j);
	}
/* Householder's reduction to bidiagonal form. */
	g = x = 0.0;
	for (i=0;i<n;i++) {
		e[i] = g;
		s = 0.0;
		l = i+1;
		for (j=i;j<m;j++)
			s += (u(j, i)*u(j, i));
		if (s < tol)
			g = 0.0;
		else {
			f = u(i, i);
			g = (f < 0) ? sqrt(s) : -sqrt(s);
			h = f * g - s;
			u(i, i) = f - g;
			for (j=l;j<n;j++) {
				s = 0.0;
				for (k=i;k<m;k++)
					s += (u(k, i) * u(k, j));
				f = s / h;
				for (k=i;k<m;k++)
					u(k, j) += (f * u(k, i));
			} /* end j */
		} /* end s */
		q(i) = g;
		s = 0.0;
		for (j=l;j<n;j++)
			s += (u(i, j) * u(i, j));
		if (s < tol)
			g = 0.0;
		else {
			f = u(i, i+1);
			g = (f < 0) ? sqrt(s) : -sqrt(s);
			h = f * g - s;
			u[i][i+1] = f - g;
			for (j=l;j<n;j++)
				e[j] = u(i, j)/h;
			for (j=l;j<m;j++) {
				s = 0.0;
				for (k=l;k<n;k++)
					s += (u(j, k) * u(i, k));
				for (k=l;k<n;k++)
					u(j, k) += (s * e[k]);
			} /* end j */
		} /* end s */
		y = fabs(q(i)) + fabs(e[i]);
		if (y > x)
			x = y;
	} /* end i */

/* accumulation of right-hand transformations */
	if (withv) {
		for (i=n-1;i>=0;i--) {
			if (g != 0.0) {
				h = u(i, i+1) * g;
				for (j=l;j<n;j++)
					v(j, i) = u(i, j)/h;
				for (j=l;j<n;j++) {
					s = 0.0;
					for (k=l;k<n;k++)
						s += (u(i, k) * v(k, j));
					for (k=l;k<n;k++)
						v(k, j) += (s * v(k, i));

				} /* end j */
			} /* end g */
			for (j=l;j<n;j++)
				v(i, j) = v(j, i) = 0.0;
			v(i, i) = 1.0;
			g = e[i];
			l = i;
		} /* end i */

	} /* end withv, parens added for clarity */

/* accumulation of left-hand transformations */
	if (withu) {
		for (i=n;i<m;i++) {
			for (j=n;j<m;j++)
				u(i, j) = 0.0;
			u(i, i) = 1.0;
		}
	}
	if (withu) {
		for (i=n-1;i>=0;i--) {
			l = i + 1;
			g = q(i);
			for (j=l;j<m;j++)  /* upper limit was 'n' */
				u(i, j) = 0.0;
			if (g != 0.0) {
				h = u(i, i) * g;
				for (j=l;j<m;j++) { /* upper limit was 'n' */
					s = 0.0;
					for (k=l;k<m;k++)
						s += (u(k, i) * u(k, j));
					f = s / h;
					for (k=i;k<m;k++)
						u(k, j) += (f * u(k, i));
				} /* end j */
				for (j=i;j<m;j++)
					u(j, i) /= g;
			} /* end g */
			else {
				for (j=i;j<m;j++)
					u(j, i) = 0.0;
			}
			u(i, i) += 1.0;
		} /* end i*/
	} /* end withu, parens added for clarity */

/* diagonalization of the bidiagonal form */
	eps *= x;
	for (k=n-1;k>=0;k--) {
		iter = 0;
test_f_splitting:
		for (l=k;l>=0;l--) {
			if (fabs(e[l]) <= eps) goto test_f_convergence;
			if (fabs(q(l-1)) <= eps) goto cancellation;
		} /* end l */

/* cancellation of e[l] if l > 0 */
cancellation:
		c = 0.0;
		s = 1.0;
		l1 = l - 1;
		for (i=l;i<=k;i++) {
			f = s * e[i];
			e[i] *= c;
			if (fabs(f) <= eps) goto test_f_convergence;
			g = q(i);
			h = q(i) = sqrt(f*f + g*g);
			c = g / h;
			s = -f / h;
			if (withu) {
				for (j=0;j<m;j++) {
					y = u(j, l1);
					z = u(j, i);
					u(j, l1) = y * c + z * s;
					u(j, i) = -y * s + z * c;
				} /* end j */
			} /* end withu, parens added for clarity */
		} /* end i */
test_f_convergence:
		z = q(k);
		if (l == k) goto convergence;

/* shift from bottom 2x2 minor */
		iter++;
		if (iter > 30) {
			retval = k;
			break;
		}
		x = q(l);
		y = q(k-1);
		g = e[k-1];
		h = e[k];
		f = ((y-z)*(y+z) + (g-h)*(g+h)) / (2*h*y);
		g = sqrt(f*f + 1.0);
		f = ((x-z)*(x+z) + h*(y/((f<0)?(f-g):(f+g))-h))/x;
/* next QR transformation */
		c = s = 1.0;
		for (i=l+1;i<=k;i++) {
			g = e[i];
			y = q(i);
			h = s * g;
			g *= c;
			e[i-1] = z = sqrt(f*f+h*h);
			c = f / z;
			s = h / z;
			f = x * c + g * s;
			g = -x * s + g * c;
			h = y * s;
			y *= c;
			if (withv) {
				for (j=0;j<n;j++) {
					x = v(j, i-1);
					z = v(j, i);
					v[j][i-1] = x * c + z * s;
					v(j, i) = -x * s + z * c;
				} /* end j */
			} /* end withv, parens added for clarity */
			q[i-1] = z = sqrt(f*f + h*h);
			c = f/z;
			s = h/z;
			f = c * g + s * y;
			x = -s * g + c * y;
			if (withu) {
				for (j=0;j<m;j++) {
					y = u(j, i-1);
					z = u(j, i);
					u[j][i-1] = y * c + z * s;
					u(j, i) = -y * s + z * c;
				} /* end j */
			} /* end withu, parens added for clarity */
		} /* end i */
		e[l] = 0.0;
		e[k] = f;
		q(k) = x;
		goto test_f_splitting;
convergence:
		if (z < 0.0) {
/* q(k) is made non-negative */
			q(k) = - z;
			if (withv) {
				for (j=0;j<n;j++)
					v(j, k) = -v(j, k);
			} /* end withv, parens added for clarity */
		} /* end z */
	} /* end k */
	
	free(e);
	return retval;
}

void Mlib::svd4(const Array<double>& a, Array<double>& u, Array<double>& s, Array<double>& vT) {
	assert(a.shape(0) >= a.shape(1));
	assert(a.ndim() == 2);
	s.resize(std::min(a.shape(0), a.shape(1)));
	u.resize[a.shape(0)](std::max(a.shape(0), a.shape(1)));  // ECON: ncols = s.length()
	Array<double> v(ArrayShape{a.shape(1), a.shape(1)});  // ECON: nrows = s.length()

	int retval = svd_hac((int)a.shape(0), (int)a.shape(1), true, true, 1e-12, 1e-12, a, s, u, v);
	if (retval != 0) {
		throw std::runtime_error("svd_hac returned with code " + std::to_string(retval));
	}
	vT = v.T();
	sort_svd(u, s, vT);
}

double Mlib::cond4(const Array<double>& a) {
	assert(a.shape(0) >= a.shape(1));
	Array<double> u{ArrayShape{a.shape(1), a.shape(1)}};
	Array<double> s{ArrayShape{a.shape(1)}};
	Array<double> v;
	Array<double> m = dot(a.T(), a);
    int retval =
        svd_hac((int)a.shape(1), (int)a.shape(1), false, false, 1e-12, 1e-12, m, s, u, v);
	if (retval != 0) {
		throw std::runtime_error("svd_hac returned with code " + std::to_string(retval));
	}
	return max(s) / min(s);
}

double Mlib::cond4_x(const Array<double>& a) {
	assert(a.shape(0) >= a.shape(1));
	Array<double> u{ArrayShape{a.shape(0), std::max(a.shape(0), a.shape(1))}};  // ECON: ncols = s.length()
	Array<double> s{ArrayShape{std::min(a.shape(0), a.shape(1))}};
	Array<double> v;
    int retval =
        svd_hac((int)a.shape(0), (int)a.shape(1), false, false, 1e-12, 1e-12, a, s, u, v);
	if (retval != 0) {
		throw std::runtime_error("svd_hac returned with code " + std::to_string(retval));
	}
	return squared(max(s) / min(s));
}
