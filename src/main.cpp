#include <cstdio>
#include <typeinfo>

#include "lib.h"

using namespace aerobus;

int test_type_at() {
	if (!std::is_same<type_at_t<0, float, int, long>, float>::value) {
		return 1;
	}
	if (!std::is_same<type_at_t<1, float, int, long>, int>::value) {
		return 1;
	}
	if (!std::is_same<type_at_t<2, float, int, long>, long>::value) {
		return 1;
	}

	return 0;
}

int test_poly_simplify() {
	using poly1 = polynomial<i32>::val<i32::val<0>, i32::val<1>, i32::val<2>>;
	using simplified1 = poly_simplify_t<i32, poly1>;
	using expected1 = polynomial<i32>::val<i32::val<1>, i32::val<2>>;
	if (!std::is_same<expected1, simplified1>::value) {
		return 1;
	}

	using poly2 = polynomial<i32>::val<i32::val<12>>;
	using simplified2 = poly_simplify_t<i32, poly2>;
	if (!std::is_same<poly2, simplified2>::value) {
		return 1;
	}


	return 0;
}

int test_poly_eval() {
	// 1 + 2x + 3x^2
	using poly = polynomial<i32>::val<i32::val<3>, i32::val<2>, i32::val<1>>;
	constexpr int v = poly::eval(1);
	if (v != 6) {
		return 1;
	}
	constexpr float vv = poly::eval(1.0f);
	if (vv != 6.0f) {
		return 1;
	}

	// 1/2 + 3x/2
	using polyf = polynomial<Q32>::val<Q32::val<i32::val<3>, i32::val<2>>, Q32::val<i32::val<1>, i32::val<2>>>;
	constexpr float vvv = polyf::eval(1.0f);
	if (vvv != 2.0f) {
		return 1;
	}
	constexpr double vvvv = polyf::eval(-1.0);
	if (vvvv != -1.0) {
		return 1;
	}

	return 0;
}

int test_fraction_field_eval() {
	using half = Q32::val<i32::one, i32::val<2>>;
	constexpr float x = half::eval(2.0f);
	if (x != 0.5f) {
		return 1;
	}
	using thirdhalf = Q32::val<i32::val<3>, i32::val<2>>;
	constexpr float y = thirdhalf::eval(1.0f);
	if (y != 1.5f) {
		return 1;
	}

	// 3/2 + x / 2
	using polyA = polynomial<Q32>::val<half, thirdhalf>;
	constexpr float a = polyA::eval(2.0f);
	if (a != 2.5F) {
		return 1;
	}
	// 1/2 + x
	using polyB = polynomial<Q32>::val<Q32::one, half>;
	using F = FPQ32::val<polyA, polyB>;
	constexpr float z = F::eval(2.0f);
	if (z != 1.0f) {
		return 1;
	}
	constexpr float zz = F::eval(-1.0f);
	if (zz != -2.0f) {
		return 1;
	}


	return 0;
}

int test_coeff_at() {
	// 1 + 2x + 3x^2
	using poly = polynomial<i32>::val<i32::val<3>, i32::val<2>, i32::val<1>>;
	using at0 = poly::coeff_at_t<0>;
	using at1 = poly::coeff_at_t<1>;
	using at2 = poly::coeff_at_t<2>;
	using at3 = poly::coeff_at_t<3>;

	if(!std::is_same<at0, i32::val<1>>::value) {
		return 1;
	}

	if (!std::is_same<at1, i32::val<2>>::value) {
		return 1;
	}

	if (!std::is_same<at2, i32::val<3>>::value) {
		return 1;
	}

	if (!std::is_same<at3, i32::val<0>>::value) {
		return 1;
	}

	return 0;
}

int test_poly_add_at() {
	{
		using P1 = polynomial<i32>::val<i32::val<1>, i32::val<2>>;
		using P2 = polynomial<i32>::val<i32::val<2>, i32::val<3>>;
		using add0 = poly_add_at_t<i32, P1, P2, 0>;
		using add1 = poly_add_at_t<i32, P1, P2, 1>;
		using add2 = poly_add_at_t<i32, P1, P2, 2>;
		{
			auto expected = 5;
			auto actual = add0::v;
			if (expected != actual) {
				printf("expected %d -- got %d\n", expected, actual);
				return 1;
			}
		} 
		{
			auto expected = 3;
			auto actual = add1::v;
			if (expected != actual) {
				printf("expected %d -- got %d\n", expected, actual);
				return 1;
			}
		}
		{
			auto expected = 0;
			auto actual = add2::v;
			if (expected != actual) {
				printf("expected %d -- got %d\n", expected, actual);
				return 1;
			}
		}
	}

	return 0;
}


template<typename... coeffs>
using IX = polynomial<i32>::val<coeffs...>;
template<int32_t x>
using Int = i32::val<x>;

template<typename P1, typename P2>
using add_ix = poly_add_t<i32, P1, P2>;
template<typename P1, typename P2>
using sub_ix = poly_sub_t<i32, P1, P2>;

int test_poly_add() {
	{
		// 1 + x
		using P1 = IX<Int<1>, Int<1>>;
		// 1 + x + x^2
		using P2 = IX<Int<1>, Int<1>, Int<1>>;
		// 2 + 2x + x^2
		using A = polynomial<i32>::add_t<P1, P2>;

		if (A::coeff_at_t<0>::v != 2) {
			return 1;
		}
		if (A::coeff_at_t<1>::v != 2) {
			return 1;
		}
		if (A::coeff_at_t<2>::v != 1) {
			return 1;
		}
		if (A::degree != 2) {
			return 1;
		}
	}

	{
		// 1 + x - x^2
		using P1 = polynomial<i32>::val<i32::val<-1>, i32::val<1>, i32::val<1>>;
		// 1 + x + x^2
		using P2 = polynomial<i32>::val<i32::val<1>, i32::val<1>, i32::val<1>>;
		// 2 + 2x
		using A = poly_add_t<i32, P1, P2>;
		if (A::coeff_at_t<0>::v != 2) {
			return 1;
		}
		if (A::coeff_at_t<1>::v != 2) {
			return 1;
		}
		if (A::coeff_at_t<2>::v != 0) {
			return 1;
		}
		if (A::degree != 1) {
			return 1;
		}
	}

	return 0;
}

int test_poly_sub() {
	{
		// 1 + x
		using P1 = IX<Int<1>, Int<1>>;
		// 1 + x + x^2
		using P2 = IX<Int<1>, Int<1>, Int<1>>;
		// -x2
		using A = sub_ix<P1, P2>;

		if (A::coeff_at_t<0>::v != 0) {
			return 1;
		}
		if (A::coeff_at_t<1>::v != 0) {
			return 1;
		}
		if (A::coeff_at_t<2>::v != -1) {
			return 1;
		}
		if (A::degree != 2) {
			return 1;
		}
	}

	{
		// 1 + x + x^2
		using P1 = polynomial<i32>::val<i32::val<1>, i32::val<1>, i32::val<1>>;
		// 1 + x + x^2
		using P2 = polynomial<i32>::val<i32::val<1>, i32::val<1>, i32::val<1>>;
		// 0
		using A = poly_sub_t<i32, P2, P1>;
		if (A::coeff_at_t<0>::v != 0) {
			return 1;
		}
		if (A::coeff_at_t<1>::v != 0) {
			return 1;
		}
		if (A::coeff_at_t<2>::v != 0) {
			return 1;
		}
		if (A::degree != 0) {
			return 1;
		}
	}

	return 0;
}

int test_poly_eq() {
	{
		using A = polynomial<i32>::val<i32::val<1>>;
		using B = polynomial<i32>::val<i32::val<1>>;
		if (!polynomial<i32>::eq_t<A, B>::value) {
			return 1;
		}
	}
	{
		using A = polynomial<i32>::val<i32::val<1>>;
		using B = polynomial<i32>::val<i32::val<2>>;
		if (polynomial<i32>::eq_t<A, B>::value) {
			return 1;
		}
	}
	{
		using A = polynomial<i32>::val<i32::val<1>, i32::val<1>>;
		using B = polynomial<i32>::val<i32::val<1>, i32::val<1>>;
		if (!polynomial<i32>::eq_t<A, B>::value) {
			return 1;
		}
	}
	{
		using A = polynomial<i32>::val<i32::val<1>, i32::val<1>>;
		using B = polynomial<i32>::val<i32::val<1>, i32::val<2>>;
		if (polynomial<i32>::eq_t<A, B>::value) {
			return 1;
		}
	}
	{
		using A = polynomial<i32>::val<i32::val<1>, i32::val<1>>;
		using B = polynomial<i32>::val<i32::val<1>, i32::val<1>, i32::val<2>>;
		if (polynomial<i32>::eq_t<A, B>::value) {
			return 1;
		}
	}

	return 0;
}

int test_gcd() {
	using A = i32::gcd_t<i32::val<12>, i32::val<6>>;
	if (A::v != 6) {
		return 1;
	}
	using B = i32::gcd_t<i32::val<12>, i32::val<6>>;
	if (B::v != 6) {
		return 1;
	}

	using C = i32::gcd_t<i32::val<5>, i32::val<3>>;
	if (C::v != 1) {
		return 1;
	}

	return 0;
}

int test_poly_mul() {
	{
		using A = polynomial<i32>::val<i32::val<1>, i32::val<-1>>;
		using B = polynomial<i32>::val<i32::val<1>, i32::val<1>>;
		using mul = polynomial<i32>::mul_t<A, B>;
		using expected = polynomial<i32>::val<i32::val<1>, i32::zero, i32::val<-1>>;
		if (!std::is_same<expected, mul>::value) {
			return 1;
		}
	}
	{
		using A = polynomial<i32>::val<i32::val<1>, i32::val<1>>;
		using B = polynomial<i32>::val<i32::val<1>, i32::val<1>>;
		using mul = polynomial<i32>::mul_t<A, B>;
		using expected = polynomial<i32>::val<i32::val<1>, i32::val<2>, i32::val<1>>;
		if (!std::is_same<expected, mul>::value) {
			return 1;
		}
	}
	{
		using A = polynomial<i32>::val<i32::val<1>, i32::val<1>>;
		using B = polynomial<i32>::val<i32::val<2>>;
		using mul = polynomial<i32>::mul_t<A, B>;
		using expected = polynomial<i32>::val<i32::val<2>, i32::val<2>>;
		if (!std::is_same<expected, mul>::value) {
			return 1;
		}
	}

	return 0;
}

int test_monomial() {
	{
		//2x^3 + 0 + 0 + 0
		using A = polynomial<i32>::monomial_t<i32::val<2>, 3>;
		if (A::coeff_at_t<3>::v != 2) {
			return 1;
		}
		if (A::coeff_at_t<2>::v != 0) {
			return 1;
		}
		if (A::coeff_at_t<1>::v != 0) {
			return 1;
		}
		if (A::coeff_at_t<0>::v != 0) {
			return 1;
		}
	}

	return 0;
}

int test_poly_div() {
	// divisibility
	{
		// x^2 - 1
		using A = polynomial<i32>::val<i32::val<1>, i32::val<0>, i32::val<-1>>;
		// x - 1
		using B = polynomial<i32>::val<i32::val<1>, i32::val<-1>>;
		// x + 1
		using Q = polynomial<i32>::div_t<A, B>;
		using R = polynomial<i32>::mod_t<A, B>;
		if (!R::is_zero_t::value) {
			return 1;
		}
		if (Q::degree != 1) {
			return 1;
		}
		if (Q::coeff_at_t<0>::v != 1) {
			return 1;
		}
		if (Q::coeff_at_t<1>::v != 1) {
			return 1;
		}
	}
	// divide by constant
	{
		using A = polynomial<i32>::val<i32::val<2>, i32::val<2>, i32::val<2>>;
		using B = polynomial<i32>::val<i32::val<2>>;
		using C = polynomial<i32>::div_t<A, B>;
		using R = polynomial<i32>::mod_t<A, B>;
		if (!R::is_zero_t::value) {
			return 1;
		}
		if (C::degree != 2) {
			return 1;
		}
		if (C::coeff_at_t<0>::v != 1) {
			return 1;
		}
		if (C::coeff_at_t<1>::v != 1) {
			return 1;
		}
		if (C::coeff_at_t<2>::v != 1) {
			return 1;
		}
	}
	// no divisibility
	{
		using A = polynomial<i32>::val<i32::val<1>, i32::val<1>, i32::val<1>>;
		using B = polynomial<i32>::val<i32::val<1>, i32::val<1>>;
		using C = polynomial<i32>::div_t<A, B>;
		using R = polynomial<i32>::mod_t<A, B>;
		if (C::degree != 1) {
			return 1;
		}
		if (C::coeff_at_t<0>::v != 0) {
			return 1;
		}
		if (C::coeff_at_t<1>::v != 1) {
			return 1;
		}
		if (R::degree != 0) {
			return 1;
		}
		if (R::aN::v != 1) {
			return 1;
		}
	}
	// float divisibility
	{
		// x2 -1
		using A = polynomial<Q32>::val<Q32::one, Q32::zero, Q32::val<i32::val<-1>, i32::val<1>>>;
		// 2x + 2
		using B = polynomial<Q32>::val<Q32::val<i32::val<2>, i32::one>, Q32::val<i32::val<2>, i32::one>>;
		using Q = polynomial<Q32>::div_t<A, B>;

		if (Q::degree != 1) {
			return 1;
		}
		if (Q::coeff_at_t<0>::x::v != -1) {
			return 1;
		}
		if (Q::coeff_at_t<0>::y::v != 2) {
			return 1;
		}
		if (Q::coeff_at_t<1>::x::v != 1) {
			return 1;
		}
		if (Q::coeff_at_t<1>::y::v != 2) {
			return 1;
		}
	}
	// float divisibility
	{
		// x2 -1
		using A = polynomial<Q32>::val<Q32::one, Q32::one>;
		// 2x + 2
		using B = polynomial<Q32>::val<Q32::val<i32::val<2>, i32::one>, Q32::val<i32::val<2>, i32::one>>;
		using Q = polynomial<Q32>::div_t<A, B>;
		if (Q::degree != 0) {
			return 1;
		}
		if (Q::coeff_at_t<0>::x::v != 1) {
			return 1;
		}
		if (Q::coeff_at_t<0>::y::v != 2) {
			return 1;
		}
	}
	return 0;
}

int test_poly_gcd() {
	{
		// (x+1)*(x+1)
		using A = polynomial<Q32>::val<Q32::one, Q32::val<i32::val<2>, i32::val<1>>, Q32::one>;
		// (x+1)
		using B = polynomial<Q32>::val<Q32::one, Q32::one>;
		using G = gcd<polynomial<Q32>>::type<A, B>;
		if (G::degree != 1) {
			return 1;
		}
		if (G::coeff_at_t<0>::x::v != 1 || G::coeff_at_t<0>::y::v != 1) {
			return 1;
		}
		if (G::coeff_at_t<1>::x::v != 1 || G::coeff_at_t<1>::y::v != 1) {
			return 1;
		}
	}
	{
		// (x+1)*(x+1)
		using A = polynomial<Q32>::val<Q32::one, Q32::val<i32::val<2>, i32::val<1>>, Q32::one>;
		// (x+1)*(x-1) :: x^2 - 1
		using B = polynomial<Q32>::val<Q32::one, Q32::zero, Q32::val<i32::val<-1>, i32::val<1>>>;
		// x + 1
		using G = polynomial<Q32>::gcd_t<A, B>;
		if (G::degree != 1) {
			return 1;
		}
		if (G::coeff_at_t<0>::x::v != 1 || G::coeff_at_t<0>::y::v != 1) {
			return 1;
		}
		if (G::coeff_at_t<1>::x::v != 1 || G::coeff_at_t<1>::y::v != 1) {
			return 1;
		}
	}

	return 0;
}

int test_poly_to_string() {
	using P32 = polynomial<Q32>;
	using A = FPQ32::val<P32::val<Q32::one, Q32::one>, P32::val<Q32::val<i32::val<2>, i32::val<1>>, Q32::one>>;
	auto rep = A::to_string();
	if (strcmp(rep.c_str(), "(x + 1) / (2 x + 1)") != 0) {
		return 1;
	}

	return 0;
}

int test_add_q32() {
	{
		using a = Q32::val<i32::val<1>, i32::val<2>>;
		using b = Q32::val<i32::val<1>, i32::val<4>>;

		using c = Q32::add_t<a, b>;

		auto x = c::x::v;
		auto y = c::y::v;
		if (x != 3 || y != 4) {
			return 1;
		}
	}
	{
		using a = Q32::val<i32::val<1>, i32::val<2>>;
		using b = Q32::val<i32::val<1>, i32::val<3>>;

		using c = Q32::add_t<a, b>;

		auto x = c::x::v;
		auto y = c::y::v;
		if (x != 5 || y != 6) {
			return 1;
		}
	}
	{
		using a = Q32::val<i32::val<-1>, i32::val<2>>;
		using b = Q32::val<i32::val<1>, i32::val<2>>;

		using c = Q32::add_t<a, b>;

		auto x = c::x::v;
		auto y = c::y::v;
		if (x != 0 || y != 1) {
			return 1;
		}
	}

	return 0;
}

int test_sub_q32() {
	{
		using a = Q32::val<i32::val<1>, i32::val<2>>;
		using b = Q32::val<i32::val<1>, i32::val<4>>;

		using c = Q32::sub_t<a, b>;

		auto x = c::x::v;
		auto y = c::y::v;
		if (x != 1 || y != 4) {
			return 1;
		}
	}
	{
		using a = Q32::val<i32::val<1>, i32::val<2>>;
		using b = Q32::val<i32::val<1>, i32::val<3>>;

		using c = Q32::sub_t<a, b>;

		auto x = c::x::v;
		auto y = c::y::v;
		if (x != 1 || y != 6) {
			return 1;
		}
	}
	{
		using a = Q32::val<i32::val<1>, i32::val<2>>;
		using b = Q32::val<i32::val<1>, i32::val<2>>;

		using c = Q32::sub_t<a, b>;

		auto x = c::x::v;
		auto y = c::y::v;
		if (x != 0 || y != 1) {
			return 1;
		}
	}

	return 0;
}

int test_mul_q32() {
	{
		using a = Q32::val<i32::val<1>, i32::val<2>>;
		using b = Q32::val<i32::val<1>, i32::val<4>>;

		using c = Q32::mul_t<a, b>;

		auto x = c::x::v;
		auto y = c::y::v;
		if (x != 1 || y != 8) {
			return 1;
		}
	}
	{
		using a = Q32::val<i32::val<1>, i32::val<2>>;
		using b = Q32::val<i32::val<1>, i32::val<3>>;

		using c = Q32::mul_t<a, b>;

		auto x = c::x::v;
		auto y = c::y::v;
		if (x != 1 || y != 6) {
			return 1;
		}
	}
	{
		using a = Q32::val<i32::val<1>, i32::val<2>>;
		using b = Q32::val<i32::val<0>, i32::val<2>>;

		using c = Q32::mul_t<a, b>;

		auto x = c::x::v;
		auto y = c::y::v;
		if (x != 0 || y != 1) {
			return 1;
		}
	}

	return 0;
}

int test_div_q32() {
	{
		using a = Q32::val<i32::val<1>, i32::val<2>>;
		using b = Q32::val<i32::val<1>, i32::val<4>>;

		using c = Q32::div_t<a, b>;

		auto x = c::x::v;
		auto y = c::y::v;
		if (x != 2 || y != 1) {
			return 1;
		}
	}
	{
		using a = Q32::val<i32::val<1>, i32::val<2>>;
		using b = Q32::val<i32::val<1>, i32::val<3>>;

		using c = Q32::div_t<a, b>;

		auto x = c::x::v;
		auto y = c::y::v;
		if (x != 3 || y != 2) {
			return 1;
		}
	}
	{
		using a = Q32::val<i32::val<0>, i32::val<2>>;
		using b = Q32::val<i32::val<1>, i32::val<2>>;

		using c = Q32::div_t<a, b>;

		auto x = c::x::v;
		auto y = c::y::v;
		if (x != 0 || y != 1) {
			return 1;
		}
	}
	{
		using a = Q32::val<i32::val<1>, i32::val<1>>;
		using b = Q32::val<i32::val<2>, i32::val<1>>;

		using c = Q32::div_t<a, b>;

		auto x = c::x::v;
		auto y = c::y::v;
		if (x != 1 || y != 2) {
			return 1;
		}
	}

	return 0;
}

int test_simplify_q32() {
	using A = Q32::val<i32::val<2>, i32::val<2>>;
	using B = Q32::val<i32::val<1>, i32::val<1>>;
	using C = Q32::val<i32::val<-1>, i32::val<-1>>;
	using D = Q32::val<i32::val<1>, i32::val<-2>>;
	if (!Q32::eq_t<Q32::simplify_t<A>, Q32::one>::value) {
		return 1;
	}
	if (!Q32::eq_t<Q32::simplify_t<B>, Q32::one>::value) {
		return 1;
	}
	if (!Q32::eq_t<Q32::simplify_t<C>, Q32::one>::value) {
		return 1;
	}
	if (!Q32::eq_t<Q32::simplify_t<D>, Q32::val<i32::val<-1>, i32::val<2>>>::value) {
		return 1;
	}

	return 0;
}

int test_eq_q32() {
	using A = Q32::val<i32::val<2>, i32::val<2>>;
	using B = Q32::val<i32::val<1>, i32::val<1>>;
	using C = Q32::val<i32::val<-1>, i32::val<-1>>;
	if (!Q32::eq_t<A, B>::value) {
		return 1;
	}
	if (!Q32::eq_t<A, C>::value) {
		return 1;
	}
	return 0;
}

int test_fraction_field_of_fraction_field () {
	using qq32 = FractionField<Q32>;
	if (!std::is_same<Q32, qq32>::value) {
		return 1;
	}

	return 0;
}

int test_factorial() {
	constexpr float x = factorial<i32, 3>::value;
	if (x != 6.0f) {
		return 1;
	}

	constexpr size_t y = factorial<i32, 0>::value;
	if (y != 1) {
		return 1;
	}

	return 0;
}

int test_combination() {
	constexpr int x = combination<i32, 2, 4>::value;
	if (x != 6) {
		return 1;
	}
	constexpr int y = combination<i32, 0, 4>::value;
	if (y != 1) {
		return 1;
	}
	constexpr int z = combination<i32, 1, 4>::value;
	if (z != 4) {
		return 1;
	}

	constexpr int zz = combination<i32, 3, 4>::value;
	if (zz != 4) {
		return 1;
	}
	return 0;
}

int test_bernouilli() {
	constexpr float b0 = bernouilli<i32, 0>::template value<float>;
	if (b0 != 1.0f) {
		return 1;
	}
	constexpr float b1 = bernouilli<i32, 1>::template value<float>;
	if (b1 != -0.5f) {
		return 1;
	}
	using B2 = bernouilli<i32, 2>::type;
	if (B2::x::v != 1 || B2::y::v != 6) {
		return 1;
	}
	constexpr double b3 = bernouilli<i32, 3>::template value<double>;
	if (b3 != 0.0) {
		return 1;
	}

	return 0;
}

int test_exp() {
	using E = aerobus::exp<i32, 8>;
	constexpr float e0 = E::eval(0.0F);
	constexpr float e01 = E::eval(0.1F);
	if (e0 != 1.0F) {
		return 1;
	}

	if (std::abs(std::exp(0.1f) - e01) > 1E-7F) {
		return 1;
	}

	return 0;
}

int test_alternate() {
	constexpr int a0 = alternate<i32, 0>::value;
	if (a0 != 1) {
		return 1;
	}
	constexpr int a1 = alternate<i32, 1>::value;
	if (a1 != -1) {
		return 1;
	}
	constexpr int a2 = alternate<i32, 2>::value;
	if (a2 != 1) {
		return 1;
	}

	return 0;
}

int main(int argc, char* argv[]) {
	if (test_type_at() != 0) {
		return 1;
	}
	if (test_poly_simplify() != 0) {
		return 1;
	}
	if (test_coeff_at() != 0) {
		return 1;
	}
	if (test_poly_add_at() != 0) {
		return 1;
	}
	if (test_poly_add() != 0) {
		return 1;
	}
	if (test_poly_sub() != 0) {
		return 1;
	}
	if (test_poly_eq() != 0) {
		return 1;
	}
	if (test_gcd() != 0) {
		return 1;
	}
	if (test_poly_mul() != 0) {
		return 1;
	}
	if (test_poly_div() != 0) {
		return 1;
	}
	if (test_poly_to_string() != 0) {
		return 1;
	}
	if (test_monomial() != 0) {
		return 1;
	}
	if (test_poly_gcd() != 0) {
		return 1;
	}
	if (test_poly_eval() != 0) {
		return 1;
	}
	if (test_add_q32() != 0) {
		return 1;
	}
	if (test_fraction_field_eval() != 0) {
		return 1;
	}
	if (test_sub_q32() != 0) {
		return 1;
	}
	if (test_mul_q32() != 0) {
		return 1;
	}
	if (test_div_q32() != 0) {
		return 1;
	}
	if (test_eq_q32() != 0) {
		return 1;
	}
	if (test_fraction_field_of_fraction_field() != 0) {
		return 1;
	}
	if (test_factorial() != 0) {
		return 1;
	}
	if (test_combination() != 0) {
		return 1;
	}
	if (test_bernouilli() != 0) {
		return 1;
	}
	if (test_alternate() != 0) {
		return 1;
	}
	if (test_exp() != 0) {
		return 1;
	}
	return 0;
}