// -*- lsst-c++ -*-

#include <cstdint> // NOLINT(clang-diagnostic-pragma-pack)
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>
#include <algorithm>
#include <functional>
#include <string>
#include <concepts>


#ifdef _MSC_VER
#define ALIGNED(x) __declspec(align(x))
#define INLINED __forceinline 
#else 
#define ALIGNED(x) __attribute__((aligned(x)))
#define INLINED __attribute__((always_inline))  
#endif

// aligned allocation
namespace aerobus {
	/**
	 * aligned allocation of count elements of type T
	 * @tparam T the type of elements to store
	 * @param count the number of elements
	 * @param alignment alignment boundary
	*/
	template<typename T>
	T* aligned_malloc(size_t count, size_t alignment) {
		#ifdef _MSC_VER
		return static_cast<T*>(_aligned_malloc(count * sizeof(T), alignment));
		#else
		return static_cast<T*>(aligned_alloc(alignment, count * sizeof(T)));
		#endif
	}
}

// concepts
namespace aerobus
{
	template <typename R>
	concept IsRing = requires {
		typename R::one;
		typename R::zero;
		typename R::template add_t<typename R::one, typename R::one>;
		typename R::template sub_t<typename R::one, typename R::one>;
		typename R::template mul_t<typename R::one, typename R::one>;
	};
	
	template <typename R>
	concept IsEuclideanDomain = IsRing<R> && requires {
		typename R::template div_t<typename R::one, typename R::one>;
		typename R::template mod_t<typename R::one, typename R::one>;
		typename R::template gcd_t<typename R::one, typename R::one>;
		typename R::template eq_t<typename R::one, typename R::one>;
		typename R::template pos_t<typename R::one>;
		R::is_euclidean_domain == true;
	};

	template<typename R>
	concept IsField = IsEuclideanDomain<R> && requires {
		R::is_field == true;
	};
}

// utilities
namespace aerobus {
	namespace internal
	{
		template<template<typename...> typename TT, typename T>
		struct is_instantiation_of : std::false_type { };

		template<template<typename...> typename TT, typename... Ts>
		struct is_instantiation_of<TT, TT<Ts...>> : std::true_type { };

		template<template<typename...> typename TT, typename T>
		inline constexpr bool is_instantiation_of_v = is_instantiation_of<TT, T>::value;

		template <int64_t i, typename T, typename... Ts>
		struct type_at
		{
			static_assert(i < sizeof...(Ts) + 1, "index out of range");
			using type = typename type_at<i - 1, Ts...>::type;
		};

		template <typename T, typename... Ts> struct type_at<0, T, Ts...> {
			using type = T;
		};

		template <size_t i, typename... Ts>
		using type_at_t = typename type_at<i, Ts...>::type;


		template<int32_t n, int32_t i, typename E = void>
		struct _is_prime {};

		template<int32_t n, int32_t i>
		struct _is_prime<n, i, std::enable_if_t<(n == 2 || n == 3)>> : std::true_type {};

		template<int32_t n, int32_t i>
		struct _is_prime<n, i, std::enable_if_t<(n != 2 && n != 3) && (n <= 1 || n % 2 == 0 || n % 3 == 0)>> : std::false_type {};

		template<int32_t n, int32_t i>
		struct _is_prime<n, i, std::enable_if_t<
			(n > 1 && n % 2 != 0 && n % 3 != 0) &&
			(i >= 5 && i * i <= n) &&
			(n% i == 0 || n % (i + 2) == 0)>> : std::false_type {};


		template<int32_t n, int32_t i>
		struct _is_prime<n, i, std::enable_if_t<
			(n > 1 && n % 2 != 0 && n % 3 != 0) &&
			(i >= 5 && i * i <= n) &&
			(n% i != 0 && n % (i + 2) != 0)>> {
			static constexpr bool value = _is_prime<n, i + 6>::value;
		};

		template<int32_t n, int32_t i>
		struct _is_prime<n, i, std::enable_if_t<
			(n > 1 && n % 2 != 0 && n % 3 != 0) &&
			(i >= 5 && i * i > n)>> : std::true_type {};
	}

	/**
	 * check if integer is prime or not
	 * 
	 * @tparam n integer to check
	*/
	template<int32_t n>
	struct is_prime {
		/// @brief true iff n is prime
		static constexpr bool value = internal::_is_prime<n, 5>::value;
	};

	namespace internal {
		template <std::size_t... Is>
		constexpr auto index_sequence_reverse(std::index_sequence<Is...> const&)
			-> decltype(std::index_sequence<sizeof...(Is) - 1U - Is...>{});

		template <std::size_t N>
		using make_index_sequence_reverse
			= decltype(index_sequence_reverse(std::make_index_sequence<N>{}));

		/**
		 * computes the greatest common divisor 
		 * exposes it in gcd<A, B>::type
		 * as long as Ring type is an integral domain
		*/
		template<typename Ring, typename E = void>
		struct gcd;

		template<typename Ring>
		struct gcd<Ring, std::enable_if_t<Ring::is_euclidean_domain>> {
			template<typename A, typename B, typename E = void>
			struct gcd_helper {};

			// B = 0, A > 0
			template<typename A, typename B>
			struct gcd_helper<A, B, std::enable_if_t<
				((B::is_zero_t::value) &&
					(Ring::template gt_t<A, typename Ring::zero>::value))>>
			{
				using type = A;
			};

			// B = 0, A < 0
			template<typename A, typename B>
			struct gcd_helper<A, B, std::enable_if_t<
				((B::is_zero_t::value) &&
					!(Ring::template gt_t<A, typename Ring::zero>::value))>>
			{
				using type = typename Ring::template sub_t<typename Ring::zero, A>;
			};

			// B != 0
			template<typename A, typename B>
			struct gcd_helper<A, B, std::enable_if_t<
				(!B::is_zero_t::value)
				>> {
			private:
				// A / B
				using k = typename Ring::template div_t<A, B>;
				// A - (A/B)*B = A % B
				using m = typename Ring::template sub_t<A, typename Ring::template mul_t<k, B>>;
			public:
				using type = typename gcd_helper<B, m>::type;
			};

			template<typename A, typename B>
			using type = typename gcd_helper<A, B>::type;
		};
	}

	template<typename T, typename A, typename B>
	using gcd_t = typename internal::gcd<T>::template type<A, B>;
}

// type_list
namespace aerobus
{
	/**
	 * Empty pure template struct to handle type lists
	*/
    template <typename... Ts>
    struct type_list;

    namespace internal
    {
        template <typename T, typename... Us>
        struct pop_front_h
        {
            using tail = type_list<Us...>;
            using head = T;
        };

        template <uint64_t index, typename L1, typename L2>
        struct split_h
        {
        private:
            static_assert(index <= L2::length, "index ouf of bounds");
            using a = typename L2::pop_front::type;
            using b = typename L2::pop_front::tail;
            using c = typename L1::template push_back<a>;

        public:
            using head = typename split_h<index - 1, c, b>::head;
            using tail = typename split_h<index - 1, c, b>::tail;
        };

        template <typename L1, typename L2>
        struct split_h<0, L1, L2>
        {
            using head = L1;
            using tail = L2;
        };

        template <uint64_t index, typename L, typename T>
        struct insert_h
        {
            static_assert(index <= L::length, "index ouf of bounds");
            using s = typename L::template split<index>;
            using left = typename s::head;
            using right = typename s::tail;
            using ll = typename left::template push_back<T>;
            using type = typename ll::template concat<right>;
        };

        template <uint64_t index, typename L>
        struct remove_h
        {
            using s = typename L::template split<index>;
            using left = typename s::head;
            using right = typename s::tail;
            using rr = typename right::pop_front::tail;
            using type = typename left::template concat<rr>;
        };
    }

    template <typename... Ts>
    struct type_list
    {
    private:
        template <typename T>
        struct concat_h;

        template <typename... Us>
        struct concat_h<type_list<Us...>>
        {
            using type = type_list<Ts..., Us...>;
        };

    public:
        static constexpr size_t length = sizeof...(Ts);

        template <typename T>
        using push_front = type_list<T, Ts...>;

        template <uint64_t index>
        using at = internal::type_at_t<index, Ts...>;

        struct pop_front
        {
            using type = typename internal::pop_front_h<Ts...>::head;
            using tail = typename internal::pop_front_h<Ts...>::tail;
        };

        template <typename T>
        using push_back = type_list<Ts..., T>;

        template <typename U>
        using concat = typename concat_h<U>::type;

        template <uint64_t index>
        struct split
        {
        private:
            using inner = internal::split_h<index, type_list<>, type_list<Ts...>>;

        public:
            using head = typename inner::head;
            using tail = typename inner::tail;
        };

        template <uint64_t index, typename T>
        using insert = typename internal::insert_h<index, type_list<Ts...>, T>::type;

        template <uint64_t index>
        using remove = typename internal::remove_h<index, type_list<Ts...>>::type;
    };

    template <>
    struct type_list<>
    {
        static constexpr size_t length = 0;

        template <typename T>
        using push_front = type_list<T>;

        template <typename T>
        using push_back = type_list<T>;

        template <typename U>
        using concat = U;

        // TODO: assert index == 0
        template <uint64_t index, typename T>
        using insert = type_list<T>;
    };
}

// i32
namespace aerobus {
	/**
	 * 32 bits signed integers, seen as a algebraic ring with related operations
	*/
    struct i32 {
		using inner_type = int32_t;
		template<int32_t x>
		struct val {
			static constexpr int32_t v = x;

			template<typename valueType>
			static constexpr valueType get() { return static_cast<valueType>(x); }

			using is_zero_t = std::bool_constant<x == 0>;
			static std::string to_string() {
				return std::to_string(x);
			}

			template<typename valueRing>
			static constexpr valueRing eval(const valueRing& v) {
				return static_cast<valueRing>(x);
			}
		};

		using zero = val<0>;
		using one = val<1>;
		static constexpr bool is_field = false;
		static constexpr bool is_euclidean_domain = true;
		template<auto x>
		using inject_constant_t = val<static_cast<int32_t>(x)>;
		template<typename v>
		using inject_ring_t = v;

	private:
		template<typename v1, typename v2>
		struct add {
			using type = val<v1::v + v2::v>;
		};

		template<typename v1, typename v2>
		struct sub {
			using type = val<v1::v - v2::v>;
		};

		template<typename v1, typename v2>
		struct mul {
			using type = val<v1::v* v2::v>;
		};

		template<typename v1, typename v2>
		struct div {
			using type = val<v1::v / v2::v>;
		};

		template<typename v1, typename v2>
		struct remainder {
			using type = val<v1::v % v2::v>;
		};

		template<typename v1, typename v2>
		struct gt {
			using type = std::conditional_t<(v1::v > v2::v), std::true_type, std::false_type>;
		};

		template<typename v1, typename v2>
		struct lt {
			using type = std::conditional_t<(v1::v < v2::v), std::true_type, std::false_type>;
		};

		template<typename v1, typename v2>
		struct eq {
			using type = std::conditional_t<(v1::v == v2::v), std::true_type, std::false_type>;
		};

		template<typename v1>
		struct pos {
			using type = std::bool_constant<(v1::v > 0)>;
		};

	public:
		template<typename v1, typename v2>
		using add_t = typename add<v1, v2>::type;

		template<typename v1, typename v2>
		using sub_t = typename sub<v1, v2>::type;

		template<typename v1, typename v2>
		using mul_t = typename mul<v1, v2>::type;

		template<typename v1, typename v2>
		using div_t = typename div<v1, v2>::type;

		template<typename v1, typename v2>
		using mod_t = typename remainder<v1, v2>::type;

		template<typename v1, typename v2>
		using gt_t = typename gt<v1, v2>::type;

		template<typename v1, typename v2>
		using lt_t = typename lt<v1, v2>::type;

		template<typename v1, typename v2>
		using eq_t = typename eq<v1, v2>::type;

		template<typename v1, typename v2>
		using gcd_t = gcd_t<i32, v1, v2>;

		template<typename v>
		using pos_t = typename pos<v>::type;
	};
}

// i64
namespace aerobus {
	/**
	 * 64 bits signed integers, seen as a algebraic ring with related operations
	*/
    struct i64 {
		using inner_type = int64_t;
		template<int64_t x>
		struct val {
			static constexpr int64_t v = x;

			template<typename valueType>
			static constexpr valueType get() { return static_cast<valueType>(x); }

			using is_zero_t = std::bool_constant<x == 0>;
			static std::string to_string() {
				return std::to_string(x);
			}

			template<typename valueRing>
			static constexpr valueRing eval(const valueRing& v) {
				return static_cast<valueRing>(x);
			}
		};

		template<auto x>
		using inject_constant_t = val<static_cast<int64_t>(x)>;

		template<typename v>
		using inject_ring_t = v;

		using zero = val<0>;
		using one = val<1>;
		static constexpr bool is_field = false;
		static constexpr bool is_euclidean_domain = true;

	private:
		template<typename v1, typename v2>
		struct add {
			using type = val<v1::v + v2::v>;
		};

		template<typename v1, typename v2>
		struct sub {
			using type = val<v1::v - v2::v>;
		};

		template<typename v1, typename v2>
		struct mul {
			using type = val<v1::v* v2::v>;
		};

		template<typename v1, typename v2>
		struct div {
			using type = val<v1::v / v2::v>;
		};

		template<typename v1, typename v2>
		struct remainder {
			using type = val<v1::v% v2::v>;
		};

		template<typename v1, typename v2>
		struct gt {
			using type = std::conditional_t<(v1::v > v2::v), std::true_type, std::false_type>;
		};

		template<typename v1, typename v2>
		struct lt {
			using type = std::conditional_t<(v1::v < v2::v), std::true_type, std::false_type>;
		};

		template<typename v1, typename v2>
		struct eq {
			using type = std::conditional_t<(v1::v == v2::v), std::true_type, std::false_type>;
		};

		template<typename v>
		struct pos {
			using type = std::bool_constant<(v::v > 0)>;
		};	

	public:
		template<typename v1, typename v2>
		using add_t = typename add<v1, v2>::type;

		template<typename v1, typename v2>
		using sub_t = typename sub<v1, v2>::type;

		template<typename v1, typename v2>
		using mul_t = typename mul<v1, v2>::type;

		template<typename v1, typename v2>
		using div_t = typename div<v1, v2>::type;

		template<typename v1, typename v2>
		using mod_t = typename remainder<v1, v2>::type;

		template<typename v1, typename v2>
		using gt_t = typename gt<v1, v2>::type;

		template<typename v1, typename v2>
		using lt_t = typename lt<v1, v2>::type;

		template<typename v1, typename v2>
		using eq_t = typename eq<v1, v2>::type;

		template<typename v1, typename v2>
		using gcd_t = gcd_t<i64, v1, v2>;

		template<typename v>
		using pos_t = typename pos<v>::type;
	};
}

// z/pz
namespace aerobus {
	/**
	 * congruence classes of integers for a modulus
	 * if p is prime, zpz is a field, otherwise an integral domain with all related operations
	*/
    template<int32_t p>
	struct zpz {
		using inner_type = int32_t;
		template<int32_t x>
		struct val {
			static constexpr int32_t v = x % p;

			template<typename valueType>
			static constexpr valueType get() { return static_cast<valueType>(x % p); }

			using is_zero_t = std::bool_constant<x% p == 0>;
			static std::string to_string() {
				return std::to_string(x % p);
			}

			template<typename valueRing>
			static constexpr valueRing eval(const valueRing& v) {
				return static_cast<valueRing>(x % p);
			}
		};

		template<auto x>
		using inject_constant_t = val<static_cast<int32_t>(x)>;

		using zero = val<0>;
		using one = val<1>;
		static constexpr bool is_field = is_prime<p>::value;
		static constexpr bool is_euclidean_domain = true;

	private:
		template<typename v1, typename v2>
		struct add {
			using type = val<(v1::v + v2::v) % p>;
		};

		template<typename v1, typename v2>
		struct sub {
			using type = val<(v1::v - v2::v) % p>;
		};

		template<typename v1, typename v2>
		struct mul {
			using type = val<(v1::v* v2::v) % p>;
		};

		template<typename v1, typename v2>
		struct div {
			using type = val<(v1::v% p) / (v2::v % p)>;
		};

		template<typename v1, typename v2>
		struct remainder {
			using type = val<(v1::v% v2::v) % p>;
		};

		template<typename v1, typename v2>
		struct gt {
			using type = std::conditional_t<(v1::v% p > v2::v% p), std::true_type, std::false_type>;
		};

		template<typename v1, typename v2>
		struct lt {
			using type = std::conditional_t<(v1::v% p < v2::v% p), std::true_type, std::false_type>;
		};

		template<typename v1, typename v2>
		struct eq {
			using type = std::conditional_t<(v1::v% p == v2::v % p), std::true_type, std::false_type>;
		};

		template<typename v1>
		struct pos {
			using type = std::bool_constant<(v1::v > 0)>;
		};

	public:
		template<typename v1, typename v2>
		using add_t = typename add<v1, v2>::type;

		template<typename v1, typename v2>
		using sub_t = typename sub<v1, v2>::type;

		template<typename v1, typename v2>
		using mul_t = typename mul<v1, v2>::type;

		template<typename v1, typename v2>
		using div_t = typename div<v1, v2>::type;

		template<typename v1, typename v2>
		using mod_t = typename remainder<v1, v2>::type;

		template<typename v1, typename v2>
		using gt_t = typename gt<v1, v2>::type;

		template<typename v1, typename v2>
		using lt_t = typename lt<v1, v2>::type;

		template<typename v1, typename v2>
		using eq_t = typename eq<v1, v2>::type;

		template<typename v1, typename v2>
		using gcd_t = gcd_t<i32, v1, v2>;

		template<typename v1>
		using pos_t = typename pos<v1>::type;
	};
}

// polynomial
namespace aerobus {
    // coeffN x^N + ...
	/**
	 * polynomial with coefficients in Ring
	 * Ring must be an integral domain
	*/
	template<typename Ring, char variable_name = 'x'>
	requires IsEuclideanDomain<Ring>
	struct polynomial {
		static constexpr bool is_field = false;
		static constexpr bool is_euclidean_domain = Ring::is_euclidean_domain;

		template<typename coeffN, typename... coeffs>
		struct val {
			static constexpr size_t degree = sizeof...(coeffs);
			using aN = coeffN;
			using strip = val<coeffs...>;
			using is_zero_t = std::bool_constant<(degree == 0) && (aN::is_zero_t::value)>;

			template<size_t index, typename E = void>
			struct coeff_at {};

			template<size_t index>
			struct coeff_at<index, std::enable_if_t<(index >= 0 && index <= sizeof...(coeffs))>> {
				using type = internal::type_at_t<sizeof...(coeffs) - index, coeffN, coeffs...>;
			};

			template<size_t index>
			struct coeff_at<index, std::enable_if_t<(index < 0 || index > sizeof...(coeffs))>> {
				using type = typename Ring::zero;
			};

			template<size_t index>
			using coeff_at_t = typename coeff_at<index>::type;

			static std::string to_string() {
				return string_helper<coeffN, coeffs...>::func();
			}

			template<typename valueRing>
			static constexpr valueRing eval(const valueRing& x) {
				return eval_helper<valueRing, val>::template inner<0, degree + 1>::func(static_cast<valueRing>(0), x);
			}
		};

		// specialization for constants
		template<typename coeffN>
		struct val<coeffN> {
			static constexpr size_t degree = 0;
			using aN = coeffN;
			using strip = val<coeffN>;
			using is_zero_t = std::bool_constant<aN::is_zero_t::value>;

			template<size_t index, typename E = void>
			struct coeff_at {};

			template<size_t index>
			struct coeff_at<index, std::enable_if_t<(index == 0)>> {
				using type = aN;
			};

			template<size_t index>
			struct coeff_at<index, std::enable_if_t<(index < 0 || index > 0)>> {
				using type = typename Ring::zero;
			};

			template<size_t index>
			using coeff_at_t = typename coeff_at<index>::type;

			static std::string to_string() {
				return string_helper<coeffN>::func();
			}

			template<typename valueRing>
			static constexpr valueRing eval(const valueRing& x) {
				return static_cast<valueRing>(aN::template get<valueRing>());
			}
		};

		using zero = val<typename Ring::zero>;
		using one = val<typename Ring::one>;
		using X = val<typename Ring::one, typename Ring::zero>;

	private:
		template<typename P, typename E = void>
		struct simplify;

		template <typename P1, typename P2, typename I>
		struct add_low;

		template<typename P1, typename P2>
		struct add {
			using type = typename simplify<typename add_low<
			P1,
			P2,
			internal::make_index_sequence_reverse<
			std::max(P1::degree, P2::degree) + 1
			>>::type>::type;
		};

		template <typename P1, typename P2, typename I>
		struct sub_low;

		template <typename P1, typename P2, typename I>
		struct mul_low;

		template<typename v1, typename v2>
		struct mul {
				using type = typename mul_low<
					v1,
					v2,
					internal::make_index_sequence_reverse<
					v1::degree + v2::degree + 1
					>>::type;
		};

		template<typename coeff, size_t deg>
		struct monomial;

		template<typename v, typename E = void>
		struct derive_helper {};

		template<typename v>
		struct derive_helper<v, std::enable_if_t<v::degree == 0>> {
			using type = zero;
		};

		template<typename v>
		struct derive_helper<v, std::enable_if_t<v::degree != 0>> {
			using type = typename add<typename derive_helper<typename v::strip>::type,
				typename monomial<
					typename Ring::template mul_t<
						typename v::aN,
						typename Ring::template inject_constant_t<(v::degree)>
					>,
					v::degree - 1
				>::type
			>::type;
		};

		template<typename v1, typename v2, typename E = void>
		struct eq_helper {};

		template<typename v1, typename v2>
		struct eq_helper<v1, v2, std::enable_if_t<v1::degree != v2::degree>> {
			using type = std::false_type;
		};

		template<typename v1, typename v2>
		struct eq_helper<v1, v2, std::enable_if_t<
			v1::degree == v2::degree &&
			v1::degree != 0 &&
			std::is_same<
			typename Ring::template eq_t<typename v1::aN, typename v2::aN>,
			std::true_type
			>::value
		>
		> {
			using type = typename eq_helper<typename v1::strip, typename v2::strip>::type;
		};

		template<typename v1, typename v2>
		struct eq_helper<v1, v2, std::enable_if_t<
			v1::degree == v1::degree &&
			v1::degree == 0
		>
		> {
			using type = typename Ring::template eq_t<typename v1::aN, typename v2::aN>;
		};

		template<typename v1, typename v2, typename E = void>
		struct lt_helper {};

		template<typename v1, typename v2>
		struct lt_helper<v1, v2, std::enable_if_t<(v1::degree < v2::degree)>> {
			using type = std::true_type;
		};

		template<typename v1, typename v2>
		struct lt_helper<v1, v2, std::enable_if_t<(v1::degree == v2::degree)>> {
			using type = typename Ring::template lt_t<typename v1::aN, typename v2::aN>;
		};

		template<typename v1, typename v2>
		struct lt_helper<v1, v2, std::enable_if_t<(v1::degree > v2::degree)>> {
			using type = std::false_type;
		};

		template<typename v1, typename v2, typename E = void>
		struct gt_helper {};

		template<typename v1, typename v2>
		struct gt_helper<v1, v2, std::enable_if_t<(v1::degree > v2::degree)>> {
			using type = std::true_type;
		};

		template<typename v1, typename v2>
		struct gt_helper<v1, v2, std::enable_if_t<(v1::degree == v2::degree)>> {
			using type = std::false_type;
		};

		template<typename v1, typename v2>
		struct gt_helper<v1, v2, std::enable_if_t<(v1::degree < v2::degree)>> {
			using type = std::false_type;
		};

		// when high power is zero : strip
		template<typename P>
		struct simplify<P, std::enable_if_t<
			std::is_same<
			typename Ring::zero,
			typename P::aN
			>::value && (P::degree > 0)
		>>
		{
			using type = typename simplify<typename P::strip>::type;
		};

		// otherwise : do nothing
		template<typename P>
		struct simplify<P, std::enable_if_t<
			!std::is_same<
			typename Ring::zero,
			typename P::aN
			>::value && (P::degree > 0)
		>>
		{
			using type = P;
		};

		// do not simplify constants
		template<typename P>
		struct simplify<P, std::enable_if_t<P::degree == 0>> {
			using type = P;
		};

		// addition at
		template<typename P1, typename P2, size_t index>
		struct add_at {
			using type =
				typename Ring::template add_t<typename P1::template coeff_at_t<index>, typename P2::template coeff_at_t<index>>;
		};

		template<typename P1, typename P2, size_t index>
		using add_at_t = typename add_at<P1, P2, index>::type;

		template<typename P1, typename P2, std::size_t... I>
		struct add_low<P1, P2, std::index_sequence<I...>> {
			using type = val<add_at_t<P1, P2, I>...>;
		};

		// substraction at
		template<typename P1, typename P2, size_t index>
		struct sub_at {
			using type =
				typename Ring::template sub_t<typename P1::template coeff_at_t<index>, typename P2::template coeff_at_t<index>>;
		};

		template<typename P1, typename P2, size_t index>
		using sub_at_t = typename sub_at<P1, P2, index>::type;

		template<typename P1, typename P2, std::size_t... I>
		struct sub_low<P1, P2, std::index_sequence<I...>> {
			using type = val<sub_at_t<P1, P2, I>...>;
		};

		template<typename P1, typename P2>
		struct sub {
			using type = typename simplify<typename sub_low<
			P1,
			P2,
			internal::make_index_sequence_reverse<
			std::max(P1::degree, P2::degree) + 1
			>>::type>::type;
		};

		// multiplication at
		template<typename v1, typename v2, size_t k, size_t index, size_t stop>
		struct mul_at_loop_helper {
			using type = typename Ring::template add_t<
				typename Ring::template mul_t<
				typename v1::template coeff_at_t<index>,
				typename v2::template coeff_at_t<k - index>
				>,
				typename mul_at_loop_helper<v1, v2, k, index + 1, stop>::type
			>;
		};

		template<typename v1, typename v2, size_t k, size_t stop>
		struct mul_at_loop_helper<v1, v2, k, stop, stop> {
			using type = typename Ring::template mul_t<typename v1::template coeff_at_t<stop>, typename v2::template coeff_at_t<0>>;
		};

		template <typename v1, typename v2, size_t k, typename E = void>
		struct mul_at {};

		template<typename v1, typename v2, size_t k>
		struct mul_at<v1, v2, k, std::enable_if_t<(k < 0) || (k > v1::degree + v2::degree)>> {
			using type = typename Ring::zero;
		};

		template<typename v1, typename v2, size_t k>
		struct mul_at<v1, v2, k, std::enable_if_t<(k >= 0) && (k <= v1::degree + v2::degree)>> {
			using type = typename mul_at_loop_helper<v1, v2, k, 0, k>::type;
		};

		template<typename P1, typename P2, size_t index>
		using mul_at_t = typename mul_at<P1, P2, index>::type;

		template<typename P1, typename P2, std::size_t... I>
		struct mul_low<P1, P2, std::index_sequence<I...>> {
			using type = val<mul_at_t<P1, P2, I>...>;
		};

		// division helper
		template< typename A, typename B, typename Q, typename R, typename E = void>
		struct div_helper {};

		template<typename A, typename B, typename Q, typename R>
		struct div_helper<A, B, Q, R, std::enable_if_t<
			(R::degree < B::degree) ||
			(R::degree == 0 && std::is_same<typename R::aN, typename Ring::zero>::value)>> {
			using q_type = Q;
			using mod_type = R;
			using gcd_type = B;
		};

		template<typename A, typename B, typename Q, typename R>
		struct div_helper<A, B, Q, R, std::enable_if_t<
			(R::degree >= B::degree) &&
			!(R::degree == 0 && std::is_same<typename R::aN, typename Ring::zero>::value)>> {
		private:
			using rN = typename R::aN;
			using bN = typename B::aN;
			using pT = typename monomial<typename Ring::template div_t<rN, bN>, R::degree - B::degree>::type;
			using rr = typename sub<R, typename mul<pT, B>::type>::type;
			using qq = typename add<Q, pT>::type;

		public:
			using q_type = typename div_helper<A, B, qq, rr>::q_type;
			using mod_type = typename div_helper<A, B, qq, rr>::mod_type;
			using gcd_type = rr;
		};

		template<typename A, typename B>
		struct div {
			static_assert(Ring::is_euclidean_domain, "cannot divide in that type of Ring");
			using q_type = typename div_helper<A, B, zero, A>::q_type;
			using m_type = typename div_helper<A, B, zero, A>::mod_type;
		};

		
		template<typename P>
		struct make_unit {
			using type = typename div<P, val<typename P::aN>>::q_type;
		};

		template<typename coeff, size_t deg>
		struct monomial {
			using type = typename mul<X, typename monomial<coeff, deg - 1>::type>::type;
		};

		template<typename coeff>
		struct monomial<coeff, 0> {
			using type = val<coeff>;
		};

		template<typename valueRing, typename P>
		struct eval_helper
		{
			template<size_t index, size_t stop>
			struct inner {
				static constexpr valueRing func(const valueRing& accum, const valueRing& x) {
					constexpr valueRing coeff = static_cast<valueRing>(P::template coeff_at_t<P::degree - index>::template get<valueRing>());
					return eval_helper<valueRing, P>::template inner<index + 1, stop>::func(x * accum + coeff, x);
				}
			};

			template<size_t stop>
			struct inner<stop, stop> {
				static constexpr valueRing func(const valueRing& accum, const valueRing& x) {
					return accum;
				}
			};
		};

		template<typename coeff, typename... coeffs>
		struct string_helper {
			static std::string func() {
				std::string tail = string_helper<coeffs...>::func();
				std::string result = "";
				if (Ring::template eq_t<coeff, typename Ring::zero>::value) {
					return tail;
				}
				else if (Ring::template eq_t<coeff, typename Ring::one>::value) {
					if (sizeof...(coeffs) == 1) {
						result += std::string(1, variable_name);
					}
					else {
						result += std::string(1, variable_name) + "^" + std::to_string(sizeof...(coeffs));
					}
				}
				else {
					if (sizeof...(coeffs) == 1) {
						result += coeff::to_string() + " " + std::string(1, variable_name);
					}
					else {
						result += coeff::to_string() + " " + std::string(1, variable_name) + "^" + std::to_string(sizeof...(coeffs));
					}
				}

				if(!tail.empty()) {
					result += " + " + tail;
				}

				return result;
			}
		};

		template<typename coeff>
		struct string_helper<coeff> {
			static std::string func() {
				if(!std::is_same<coeff, typename Ring::zero>::value) {
					return coeff::to_string();
				} else {
					return "";
				}
			}
		};

	public:
		template<typename P>
		using simplify_t = typename simplify<P>::type;

		template<typename v1, typename v2>
		using add_t = typename add<v1, v2>::type;

		template<typename v1, typename v2>
		using sub_t = typename sub<v1, v2>::type;

		template<typename v1, typename v2>
		using mul_t = typename mul<v1, v2>::type;

		template<typename v1, typename v2>
		using eq_t = typename eq_helper<v1, v2>::type;

		template<typename v1, typename v2>
		using lt_t = typename lt_helper<v1, v2>::type;

		template<typename v1, typename v2>
		using gt_t = typename gt_helper<v1, v2>::type;

		template<typename v1, typename v2>
		using div_t = typename div<v1, v2>::q_type;

		template<typename v1, typename v2>
		using mod_t = typename div_helper<v1, v2, zero, v1>::mod_type;

		template<typename coeff, size_t deg>
		using monomial_t = typename monomial<coeff, deg>::type;

		template<typename v>
		using derive_t = typename derive_helper<v>::type;

		template<typename v>
		using pos_t = typename Ring::template pos_t<typename v::aN>;

		template<typename v1, typename v2>
		using gcd_t = std::conditional_t<
			Ring::is_euclidean_domain,
			typename make_unit<gcd_t<polynomial<Ring, variable_name>, v1, v2>>::type,
			void>;

		template<auto x>
		using inject_constant_t = val<typename Ring::template inject_constant_t<x>>;

		template<typename v>
		using inject_ring_t = val<v>;
	};
}

// fraction field
namespace aerobus {
    namespace internal {
        template<typename Ring, typename E = void>
		requires IsEuclideanDomain<Ring>
		struct _FractionField {};

		template<typename Ring>
		requires IsEuclideanDomain<Ring>
		struct _FractionField<Ring, std::enable_if_t<Ring::is_euclidean_domain>>
		{
			static constexpr bool is_field = true;
			static constexpr bool is_euclidean_domain = true;

			template<typename val1, typename val2, typename E = void>
			struct to_string_helper {};

			template<typename val1, typename val2>
			struct to_string_helper <val1, val2,
				std::enable_if_t<
				Ring::template eq_t<
				val2, typename Ring::one
				>::value
				>
			> {
				static std::string func() {
					return val1::to_string();
				}
			};

			template<typename val1, typename val2>
			struct to_string_helper<val1, val2,
				std::enable_if_t<
				!Ring::template eq_t<
				val2,
				typename Ring::one
				>::value
				>
			> {
				static std::string func() {
					return "(" + val1::to_string() + ") / (" + val2::to_string() + ")";
				}
			};

			template<typename val1, typename val2>
			struct val {
				using x = val1;
				using y = val2;
				using is_zero_t = typename val1::is_zero_t;
				using ring_type = Ring;
				using field_type = _FractionField<Ring>;

			 	static constexpr bool is_integer = std::is_same<val2, typename Ring::one>::value;

				template<typename valueType>
				static constexpr valueType get() { return static_cast<valueType>(x::v) / static_cast<valueType>(y::v); }

				static std::string to_string() {
					return to_string_helper<val1, val2>::func();
				}

				template<typename valueRing>
				static constexpr valueRing eval(const valueRing& v) {
					return x::eval(v) / y::eval(v);
				}
			};
			
			using zero = val<typename Ring::zero, typename Ring::one>;
			using one = val<typename Ring::one, typename Ring::one>;

			template<typename v>
			using inject_t = val<v, typename Ring::one>;

			template<auto x>
			using inject_constant_t = val<typename Ring::template inject_constant_t<x>, typename Ring::one>;

			template<typename v>
			using inject_ring_t = val<typename Ring::template inject_ring_t<v>, typename Ring::one>;

			using ring_type = Ring;

		private:
			template<typename v, typename E = void>
			struct simplify {};

			// x = 0
			template<typename v>
			struct simplify<v, std::enable_if_t<v::x::is_zero_t::value>> {
				using type = typename _FractionField<Ring>::zero;
			};

			// x != 0
			template<typename v>
			struct simplify<v, std::enable_if_t<!v::x::is_zero_t::value>> {

			private:
				using _gcd = typename Ring::template gcd_t<typename v::x, typename v::y>;
				using newx = typename Ring::template div_t<typename v::x, _gcd>;
				using newy = typename Ring::template div_t<typename v::y, _gcd>;

				using posx = std::conditional_t<!Ring::template pos_t<newy>::value, typename Ring::template sub_t<typename Ring::zero, newx>, newx>;
				using posy = std::conditional_t<!Ring::template pos_t<newy>::value, typename Ring::template sub_t<typename Ring::zero, newy>, newy>;
			public:
				using type = typename _FractionField<Ring>::template val<posx, posy>;
			};

		public:

			template<typename v>
			using simplify_t = typename simplify<v>::type;

		private:

			template<typename v1, typename v2>
			struct add {
			private:
				using a = typename Ring::template mul_t<typename v1::x, typename v2::y>;
				using b = typename Ring::template mul_t<typename v1::y, typename v2::x>;
				using dividend = typename Ring::template add_t<a, b>;
				using diviser = typename Ring::template mul_t<typename v1::y, typename v2::y>;
				using g = typename Ring::template gcd_t<dividend, diviser>;

			public:
				using type = typename _FractionField<Ring>::template simplify_t<val<dividend, diviser>>;
			};

			template<typename v>
			struct pos {
				using type = std::conditional_t<
					(Ring::template pos_t<typename v::x>::value && Ring::template pos_t<typename v::y>::value) ||
					(!Ring::template pos_t<typename v::x>::value && !Ring::template pos_t<typename v::y>::value),
					std::true_type,
					std::false_type>;

			};

			template<typename v1, typename v2>
			struct sub {
			private:
				using a = typename Ring::template mul_t<typename v1::x, typename v2::y>;
				using b = typename Ring::template mul_t<typename v1::y, typename v2::x>;
				using dividend = typename Ring::template sub_t<a, b>;
				using diviser = typename Ring::template mul_t<typename v1::y, typename v2::y>;
				using g = typename Ring::template gcd_t<dividend, diviser>;

			public:
				using type = typename _FractionField<Ring>::template simplify_t<val<dividend, diviser>>;
			};

			template<typename v1, typename v2>
			struct mul {
			private:
				using a = typename Ring::template mul_t<typename v1::x, typename v2::x>;
				using b = typename Ring::template mul_t<typename v1::y, typename v2::y>;

			public:
				using type = typename _FractionField<Ring>::template simplify_t<val<a, b>>;
			};

			template<typename v1, typename v2, typename E = void>
			struct div {};
			
			template<typename v1, typename v2>
			struct div<v1, v2, std::enable_if_t<!std::is_same<v2, typename _FractionField<Ring>::zero>::value>>  {
			private:
				using a = typename Ring::template mul_t<typename v1::x, typename v2::y>;
				using b = typename Ring::template mul_t<typename v1::y, typename v2::x>;

			public:
				using type = typename _FractionField<Ring>::template simplify_t<val<a, b>>;
			};

			template<typename v1, typename v2>
			struct div<v1, v2, std::enable_if_t<
				std::is_same<zero, v1>::value && std::is_same<v2, zero>::value>>  {
				using type = one;
			};

			template<typename v1, typename v2>
			struct eq {
				using type = std::conditional_t<
						std::is_same<typename simplify_t<v1>::x, typename simplify_t<v2>::x>::value &&
						std::is_same<typename simplify_t<v1>::y, typename simplify_t<v2>::y>::value,
					std::true_type, 
					std::false_type>;
			};

			template<typename TL, typename E = void>
			struct vadd {};

			template<typename TL>
			struct vadd<TL, std::enable_if_t<(TL::length > 1)>> {
				using head = typename TL::pop_front::type;
				using tail = typename TL::pop_front::tail;
				using type = typename add<head, typename vadd<tail>::type>::type;
			};

			template<typename TL>
			struct vadd<TL, std::enable_if_t<(TL::length == 1)>> {
				using type = typename TL::template at<0>;
			};

			template<typename... vals>
			struct vmul {};

			template<typename v1, typename... vals>
			struct vmul<v1, vals...> {
				using type = typename mul<v1, typename vmul<vals...>::type>::type;
			};

			template<typename v1>
			struct vmul<v1> {
				using type = v1;
			};
		

		public:
			template<typename v1, typename v2>
			using add_t = typename add<v1, v2>::type;
			template<typename v1, typename v2>
			using mod_t = zero;
			template<typename v1, typename v2>
			using gcd_t = v1;
			template<typename... vs>
			using vadd_t = typename vadd<vs...>::type;
			template<typename... vs>
			using vmul_t = typename vmul<vs...>::type;
			template<typename v1, typename v2>
			using sub_t = typename sub<v1, v2>::type;
			template<typename v1, typename v2>
			using mul_t = typename mul<v1, v2>::type;
			template<typename v1, typename v2>
			using div_t = typename div<v1, v2>::type;
			template<typename v1, typename v2>
			using eq_t = typename eq<v1, v2>::type;
			template<typename v1>
			using pos_t = typename pos<v1>::type;
		};

		template<typename Ring, typename E = void>
		requires IsEuclideanDomain<Ring>
		struct FractionFieldImpl {};

		// fraction field of a field is the field itself
		template<typename Field>
		requires IsEuclideanDomain<Field>
		struct FractionFieldImpl<Field, std::enable_if_t<Field::is_field>> {
			using type = Field;
			template<typename v>
			using inject_t = v;
		};

		// fraction field of a ring is the actual fraction field
		template<typename Ring>
		requires IsEuclideanDomain<Ring>
		struct FractionFieldImpl<Ring, std::enable_if_t<!Ring::is_field>> {
			using type = _FractionField<Ring>;
		};
    }

	template<typename Ring>
	requires IsEuclideanDomain<Ring>
	using FractionField = typename internal::FractionFieldImpl<Ring>::type;
}


namespace aerobus {
	/// @brief 32 bits rationals
	using q32 = FractionField<i32>;
	/// @brief polynomial with 32 bits rational coefficients
	using fpq32 = FractionField<polynomial<q32>>;
	/// @brief 64 bits rationals
	using q64 = FractionField<i64>;
	/// @brief polynomial with 64 bits rational coefficients
	using fpq64 = FractionField<polynomial<q64>>;
	/// @brief helper type : the rational V1/V2 in the field of fractions of Ring
	/// @tparam Ring the base ring
	/// @tparam v1 value 1 in Ring
	/// @tparam v2 value 2 in Ring
	template<typename Ring, typename v1, typename v2>
	using makefraction_t = typename FractionField<Ring>::template val<v1, v2>;

	template<typename Ring, typename v1, typename v2>
	using addfractions_t = typename FractionField<Ring>::template add_t<v1, v2>;
	template<typename Ring, typename v1, typename v2>
	using mulfractions_t = typename FractionField<Ring>::template mul_t<v1, v2>;
}

namespace aerobus {
	template<typename T, size_t x, typename E = void>
	struct factorial {};

	template<typename T, size_t x>
	struct factorial<T, x, std::enable_if_t<(x > 0)>> {
	private:
		template<typename, size_t, typename>
		friend struct factorial;
	public:
		using type = typename T::template mul_t<typename T::template val<x>, typename factorial<T, x - 1>::type>;
		static constexpr typename T::inner_type value = type::template get<typename T::inner_type>();
	};

	template<typename T>
	struct factorial<T, 0> {
	public:
		using type = typename T::one;
		static constexpr typename T::inner_type value = type::template get<typename T::inner_type>();
	};

	template<typename T, size_t i>
	using factorial_t = typename factorial<T, i>::type;

	namespace internal {
		template<typename T, size_t k, size_t n, typename E = void>
		struct combination_helper {};

		template<typename T, size_t k, size_t n>
		struct combination_helper<T, k, n, std::enable_if_t<(n >= 0 && k <= (n / 2) && k > 0)>> {
			using type = typename FractionField<T>::template mul_t<
				typename combination_helper<T, k - 1, n - 1>::type,
				makefraction_t<T, typename T::template val<n>, typename T::template val<k>>>;
		};

		template<typename T, size_t k, size_t n>
		struct combination_helper<T, k, n, std::enable_if_t<(n >= 0 && k > (n / 2) && k > 0)>> {
			using type = typename combination_helper<T, n - k, n>::type;
		};

		template<typename T, size_t n>
		struct combination_helper<T, 0, n> {
			using type = typename FractionField<T>::one;
		};
	}

	template<typename T, size_t k, size_t n>
	struct combination {
		using type = typename internal::combination_helper<T, k, n>::type::x;
		static constexpr typename T::inner_type value = internal::combination_helper<T, k, n>::type::template get<typename T::inner_type>();
	};

	template<typename T, size_t k, size_t n>
	using combination_t = typename combination<T, k, n>::type;


	template<typename T, size_t m>
	struct bernouilli;

	namespace internal {
		template<typename T, typename accum, size_t k, size_t m>
		struct bernouilli_helper {
			using type = typename bernouilli_helper<
				T,
				addfractions_t<T, 
					accum,
					mulfractions_t<T, 
						makefraction_t<T, 
							combination_t<T, k, m + 1>,
							typename T::one>,
						typename bernouilli<T, k>::type
					>
				>,
				k + 1,
				m>::type;
		};

		template<typename T, typename accum, size_t m>
		struct bernouilli_helper<T, accum, m, m>
		{
			using type = accum;
		};
	}

	template<typename T, size_t m>
	struct bernouilli {
		using type = typename FractionField<T>::template mul_t<
			typename internal::bernouilli_helper<T, typename FractionField<T>::zero, 0, m>::type,
			makefraction_t<T, 
			typename T::template val<static_cast<typename T::inner_type>(-1)>,
			typename T::template val<static_cast<typename T::inner_type>(m + 1)>
			>
		>;

		template<typename floatType>
		static constexpr floatType value = type::template get<floatType>();
	};

	template<typename T>
	struct bernouilli<T, 0> {
		using type = typename FractionField<T>::one;

		template<typename floatType>
		static constexpr floatType value = type::template get<floatType>();
	};

	template<typename T, size_t n>
	using bernouilli_t = typename bernouilli<T, n>::type;

	template<typename T, int k, typename E = void>
	struct alternate {};

	template<typename T, int k>
	struct alternate<T, k, std::enable_if_t<k % 2 == 0>> {
		using type = typename T::one;
		static constexpr typename T::inner_type value = type::template get<typename T::inner_type>();
	};

	template<typename T, int k>
	struct alternate<T, k, std::enable_if_t<k % 2 != 0>> {
		using type = typename T::template sub_t<typename T::zero, typename T::one>;
		static constexpr typename T::inner_type value = type::template get<typename T::inner_type>();
	};

	template<typename T, int k>
	using alternate_t = typename alternate<T, k>::type;

	template<typename T, auto p, auto n>
	struct pow {
		using type = typename T::template mul_t<typename T::template val<p>, typename pow<T, p, n - 1>::type>;
	};

	template<typename T, auto p>
	struct pow<T, p, 0> { using type = typename T::one; };

	template<typename T, auto p, auto n>
	using pow_t = typename pow<T, p, n>::type;

	namespace internal {
		template<typename, template<typename, size_t> typename, class>
		struct make_taylor_impl;

		template<typename T, template<typename, size_t> typename coeff_at, size_t... Is>
		struct make_taylor_impl<T, coeff_at, std::integer_sequence<size_t, Is...>> {
			using type = typename polynomial<FractionField<T>>::template val<typename coeff_at<T, Is>::type...>;
		};
	}

	// generic taylor serie, depending on coefficients
	template<typename T, template<typename, size_t index> typename coeff_at, size_t deg>
	using taylor = typename internal::make_taylor_impl<T, coeff_at, internal::make_index_sequence_reverse<deg + 1>>::type;

	namespace internal {
		template<typename T, size_t i>
		struct exp_coeff {
			using type = makefraction_t<T, typename T::one, factorial_t<T, i>>;
		};

		template<typename T, size_t i, typename E = void>
		struct sin_coeff_helper {};

		template<typename T, size_t i>
		struct sin_coeff_helper<T, i, std::enable_if_t<(i & 1) == 0>> {
			using type = typename FractionField<T>::zero;
		};

		template<typename T, size_t i>
		struct sin_coeff_helper<T, i, std::enable_if_t<(i & 1) == 1>> {
			using type = makefraction_t<T, alternate_t<T, i / 2>, factorial_t<T, i>>;
		};

		template<typename T, size_t i>
		struct sin_coeff {
			using type = typename sin_coeff_helper<T, i>::type;
		};

		template<typename T, size_t i, typename E = void>
		struct sh_coeff_helper {};

		template<typename T, size_t i>
		struct sh_coeff_helper<T, i, std::enable_if_t<(i & 1) == 0>> {
			using type = typename FractionField<T>::zero;
		};

		template<typename T, size_t i>
		struct sh_coeff_helper<T, i, std::enable_if_t<(i & 1) == 1>> {
			using type = makefraction_t<T, typename T::one, factorial_t<T, i>>;
		};

		template<typename T, size_t i>
		struct sh_coeff {
			using type = typename sh_coeff_helper<T, i>::type;
		};

		template<typename T, size_t i, typename E = void>
		struct cos_coeff_helper {};

		template<typename T, size_t i>
		struct cos_coeff_helper<T, i, std::enable_if_t<(i & 1) == 1>> {
			using type = typename FractionField<T>::zero;
		};

		template<typename T, size_t i>
		struct cos_coeff_helper<T, i, std::enable_if_t<(i & 1) == 0>> {
			using type = makefraction_t<T, alternate_t<T, i / 2>, factorial_t<T, i>>;
		};

		template<typename T, size_t i>
		struct cos_coeff {
			using type = typename cos_coeff_helper<T, i>::type;
		};

		template<typename T, size_t i, typename E = void>
		struct cosh_coeff_helper {};

		template<typename T, size_t i>
		struct cosh_coeff_helper<T, i, std::enable_if_t<(i & 1) == 1>> {
			using type = typename FractionField<T>::zero;
		};

		template<typename T, size_t i>
		struct cosh_coeff_helper<T, i, std::enable_if_t<(i & 1) == 0>> {
			using type = makefraction_t<T, typename T::one, factorial_t<T, i>>;
		};

		template<typename T, size_t i>
		struct cosh_coeff {
			using type = typename cosh_coeff_helper<T, i>::type;
		};

		template<typename T, size_t i>
		struct geom_coeff { using type = typename FractionField<T>::one; };


		template<typename T, size_t i, typename E = void>
		struct atan_coeff_helper;

		template<typename T, size_t i>
		struct atan_coeff_helper<T, i, std::enable_if_t<(i & 1) == 1>> {
			using type = makefraction_t<T, alternate_t<T, i / 2>, typename T::template val<i>>;
		};

		template<typename T, size_t i>
		struct atan_coeff_helper<T, i, std::enable_if_t<(i & 1) == 0>> {
			using type = typename FractionField<T>::zero;
		};

		template<typename T, size_t i>
		struct atan_coeff { using type = typename atan_coeff_helper<T, i>::type; };

		template<typename T, size_t i, typename E = void>
		struct asin_coeff_helper;

		template<typename T, size_t i>
		struct asin_coeff_helper<T, i, std::enable_if_t<(i & 1) == 1>>
		{
			using type = makefraction_t<T, 
				factorial_t<T, i - 1>,
				typename T::template mul_t<
					typename T::template val<i>,
					T::template mul_t<
						pow_t<T, 4, i / 2>,
						pow<T, factorial<T, i / 2>::value, 2
					>
				>
				>>;
		};

		template<typename T, size_t i>
		struct asin_coeff_helper<T, i, std::enable_if_t<(i & 1) == 0>>
		{
			using type = typename FractionField<T>::zero;
		};

		template<typename T, size_t i>
		struct asin_coeff {
			using type = typename asin_coeff_helper<T, i>::type;
		};

		template<typename T, size_t i>
		struct lnp1_coeff {
			using type = makefraction_t<T, 
				alternate_t<T, i + 1>,
				typename T::template val<i>>;
		};

		template<typename T>
		struct lnp1_coeff<T, 0> { using type = typename FractionField<T>::zero; };

		template<typename T, size_t i, typename E = void>
		struct asinh_coeff_helper;

		template<typename T, size_t i>
		struct asinh_coeff_helper<T, i, std::enable_if_t<(i & 1) == 1>>
		{
			using type = makefraction_t<T, 
				typename T::template mul_t<
					alternate_t<T, i / 2>,
					factorial_t<T, i - 1>
				>,
				typename T::template mul_t<
					T::template mul_t<
						typename T::template val<i>,
						pow_t<T, (factorial<T, i / 2>::value), 2>
					>,
					pow_t<T, 4, i / 2>
				>
			>;
		};

		template<typename T, size_t i>
		struct asinh_coeff_helper<T, i, std::enable_if_t<(i & 1) == 0>>
		{
			using type = typename FractionField<T>::zero;
		};

		template<typename T, size_t i>
		struct asinh_coeff {
			using type = typename asinh_coeff_helper<T, i>::type;
		};

		template<typename T, size_t i, typename E = void>
		struct atanh_coeff_helper;

		template<typename T, size_t i>
		struct atanh_coeff_helper<T, i, std::enable_if_t<(i & 1) == 1>>
		{
			// 1/i
			using type = typename FractionField<T>:: template val<
				typename T::one,
				typename T::template val<static_cast<typename T::inner_type>(i)>>;
		};

		template<typename T, size_t i>
		struct atanh_coeff_helper<T, i, std::enable_if_t<(i & 1) == 0>>
		{
			using type = typename FractionField<T>::zero;
		};

		template<typename T, size_t i>
		struct atanh_coeff {
			using type = typename asinh_coeff_helper<T, i>::type;
		};

		template<typename T, size_t i, typename E = void>
		struct tan_coeff_helper;

		template<typename T, size_t i>
		struct tan_coeff_helper<T, i, std::enable_if_t<(i % 2) == 0>> {
			using type = typename FractionField<T>::zero;
		};

		template<typename T, size_t i>
		struct tan_coeff_helper<T, i, std::enable_if_t<(i % 2) != 0>> {
		private:
			// 4^((i+1)/2)
			using _4p = typename FractionField<T>::template inject_t<pow_t<T, 4, (i + 1) / 2>>;
			// 4^((i+1)/2) - 1
			using _4pm1 = typename FractionField<T>::template sub_t<_4p, typename FractionField<T>::one>;
			// (-1)^((i-1)/2)
			using altp = typename FractionField<T>::template inject_t<alternate_t<T, (i - 1) / 2>>;
			using dividend = typename FractionField<T>::template mul_t<
				altp,
				FractionField<T>::template mul_t<
				_4p,
				FractionField<T>::template mul_t<
				_4pm1,
				bernouilli_t<T, (i + 1)>
				>
				>
			>;
		public:
			using type = typename FractionField<T>::template div_t<dividend,
				typename FractionField<T>::template inject_t<factorial_t<T, i + 1>>>;
		};

		template<typename T, size_t i>
		struct tan_coeff {
			using type = typename tan_coeff_helper<T, i>::type;
		};

		template<typename T, size_t i, typename E = void>
		struct tanh_coeff_helper;

		template<typename T, size_t i>
		struct tanh_coeff_helper<T, i, std::enable_if_t<(i % 2) == 0>> {
			using type = typename FractionField<T>::zero;
		};

		template<typename T, size_t i>
		struct tanh_coeff_helper<T, i, std::enable_if_t<(i % 2) != 0>> {
		private:
			using _4p = typename FractionField<T>::template inject_t<pow_t<T, 4, (i + 1) / 2>>;
			using _4pm1 = typename FractionField<T>::template sub_t<_4p, typename FractionField<T>::one>;
			using dividend =
				typename FractionField<T>::template mul_t<
				_4p,
				typename FractionField<T>::template mul_t<
				_4pm1,
				bernouilli_t<T, (i + 1)>
				>
				>::type;
		public:
			using type = typename FractionField<T>::template div_t<dividend,
				FractionField<T>::template inject_t<factorial_t<T, i + 1>>>;
		};

		template<typename T, size_t i>
		struct tanh_coeff {
			using type = typename tanh_coeff_helper<T, i>::type;
		};
	}

	// e^x
	template<typename T, size_t deg>
	using exp = taylor<T, internal::exp_coeff, deg>;

	// e^x - 1
	template<typename T, size_t deg>
	using expm1 = typename polynomial<FractionField<T>>::template sub_t<
		exp<T, deg>,
		typename polynomial<FractionField<T>>::one>;

	/// ln(1+x)
	template<typename T, size_t deg>
	using lnp1 = taylor<T, internal::lnp1_coeff, deg>;

	/// atan(x);
	template<typename T, size_t deg>
	using atan = taylor<T, internal::atan_coeff, deg>;

	/// sin(x)
	template<typename T, size_t deg>
	using sin = taylor<T, internal::sin_coeff, deg>;

	/// sh(x)
	template<typename T, size_t deg>
	using sinh = taylor<T, internal::sh_coeff, deg>;

	/// ch(x)
	template<typename T, size_t deg>
	using cosh = taylor<T, internal::cosh_coeff, deg>;

	/// cos(x)
	template<typename T, size_t deg>
	using cos = taylor<T, internal::cos_coeff, deg>;

	/// 1 / (1-x)
	template<typename T, size_t deg>
	using geometric_sum = taylor<T, internal::geom_coeff, deg>;

	/// asin(x)
	template<typename T, size_t deg>
	using asin = taylor<T, internal::asin_coeff, deg>;

	/// asinh(x)
	template<typename T, size_t deg>
	using asinh = taylor<T, internal::asinh_coeff, deg>;

	/// atanh(x)
	template<typename T, size_t deg>
	using atanh = taylor<T, internal::atanh_coeff, deg>;

	/// tan(x)
	template<typename T, size_t deg>
	using tan = taylor<T, internal::tan_coeff, deg>;

	/// tanh(x)
	template<typename T, size_t deg>
	using tanh = taylor<T, internal::tanh_coeff, deg>;
}

// continued fractions
namespace aerobus {
	template<int64_t... values>
	struct ContinuedFraction {};

	template<int64_t a0>
	struct ContinuedFraction<a0> {
		using type = typename q64::template inject_constant_t<a0>;
		static constexpr double val = type::template get<double>();
	};

	template<int64_t a0, int64_t... rest> 
	struct ContinuedFraction<a0, rest...> {
		using type = q64::template add_t<
				typename q64::template inject_constant_t<a0>,
				typename q64::template div_t<
					typename q64::one,
					typename ContinuedFraction<rest...>::type
				>>;
		static constexpr double val = type::template get<double>();
	};

	using PI_fraction = ContinuedFraction<3, 7, 15, 1, 292, 1, 1, 1, 2, 1, 3, 1, 14, 2, 1, 1, 2, 2, 2, 2, 1>;
	using E_fraction = ContinuedFraction<2, 1, 2, 1, 1, 4, 1, 1, 6, 1, 1, 8, 1, 1, 10, 1, 1, 12, 1, 1, 14, 1, 1>;
	using SQRT2_fraction = ContinuedFraction<1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2>;
	using SQRT3_fraction = ContinuedFraction<1, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2>;
}

// known polynomials
namespace aerobus {
	namespace internal {
		template<typename T, int deg>
		struct chebyshev_helper {
			using P = polynomial<FractionField<T>>;
			using type = P::template sub_t<
				P::template mul_t<
					P::template val<FractionField<T>::template inject_constant_t<2>>,
					typename chebyshev_helper<T, deg-1>::type;
				>,
				typename chebyshev_helper<T, deg-2>::type
			>;
		};

		template<typename T>
		struct chebyshev_helper<T, 0> {
			using type = polynomial<FractionField<T>>::one;
		};

		template<typename T>
		struct chebyshev_helper<T, 0> {
			using type = polynomial<FractionField<T>>::X;
		};
	}

	template<typename T, size_t deg>
	using chebyshev = typename internal::chebyshev_helper<T, deg>::type;
}