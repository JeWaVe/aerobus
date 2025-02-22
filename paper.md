---
title: 'Aerobus: a C++ template library for polynomials algebra over discrete Euclidean domains'
tags:
  - Polynomials
  - Mathematics
  - metaprogramming
  - Euclidean Domains
  - Rings
  - Fields
authors:
  - name: Regis Portalez
    orcid: 0000-0002-6291-3345 
    affiliation: 1
affiliations:
 - name: COMUA, France
   index: 1
date: 12 October 2023
bibliography: paper.bib
---

## Summary

C++ comes with high compile-time computations capability, also known as metaprogramming with templates.
Templates are a language-in-the-language which is Turing-complete, meaning we can run every computation at compile time instead of runtime, as long as input data is known at compile time.

Using these capabilities, vastly extended with the latest versions of the standard, we implemented a library for discrete Euclidean domains (commutative and associative), such as $\mathbb{Z}$. We also provide a way to generate the fraction field of such rings (e.g. $\mathbb{Q}$).

We also implemented polynomials over such discrete rings and fields (e.g. $\mathbb{Q}[X]$). Since polynomials are also a ring, the above implementation gives us rational fractions as the field of fractions of polynomials.

In addition, we expose a way to generate Taylor series of analytic functions, known polynomials (e.g. Chebyshev), continued fractions, quotient rings and small degree Conway polynomials to define Galois finite fields.

`Aerobus` was designed to be used in high-performance software, teaching purposes or embedded software. It compiles with major compilers: `gcc`, `clang` and `msvc`. It is quite easily configurable and extensible.

## Statement of need

By implementing general algebra concepts such as discrete rings, field of fractions and polynomials, `Aerobus` can serve multiple purposes, mainly polynomial arithmetic at compile time and efficient [@aerobus_benchmarks_2023] polynomial evaluation, regardless of the coefficients `ring`.

The main application we want to express in this paper is the automatic (and configurable) generation or Taylor approximation of analytic functions such as `exp` or `sin`, by using polynomial arithmetic at compile time.

Some important software, such as `geographiclib` [@karney2013algorithms] evaluate polynomials with a simple loop, expected to be unrolled by compiler. It works really well (on arithmetic types such as `double`) but does not provide a way to manipulate polynomials (addition, multiplication, division, modulus) automatically where `aerobus` does at no runtime cost.

Notable libraries such as [boost](https://live.boost.org/doc/libs/1_86_0/libs/math/doc/html/math_toolkit/polynomials.html) provide polynomial arithmetic, but arithmetic is done at runtime with memory allocations while `aerobus` does it at compile time.

Common analytic functions are usually exposed by the standard library (`<cmath>`) with high (guaranteed) precision. However, in high-performance computing, when not compiled with `-Ofast`, evaluating `std::exp` has several flaws:

- It leads to a `syscall` which is very expensive;
- It doesn't leverage vector units (AVX, AVX2, AVX512 or equivalent in non-intel hardware);
- Results are hardware dependent.

Hardware vendors provide high-performance libraries such as [@wang2014intel], but implementation is often hidden and not extensible.

Some others can provide vectorized functions, such as [@wang2014intel] does. But libraries like VML are highly tight to one architecture by their use of intrinsic or inline assembly. In addition, they only provide a restricted list of math functions and do not expose capabilities to generate high-performance versions of other functions such as `arctanh`. It is the same for the standard library compiled with `-Ofast`: it generates a vectorized version of some functions (such as `exp`) but with no control of precision and no extensibility. In addition, `fast-math` versions are compiler and architecture dependent, which can be a problem for results reproducibility.

`Aerobus` provides automatic generation of such functions, in a hardware-independent way, as tested on x86 and CUDA platforms. In addition, `Aerobus` provides a way to control the precision of the generated function by changing the degree of Taylor expansion, which can't be used in competing libraries without reimplementing the whole function or changing the array of coefficients.

`Aerobus` does not provide optimal approximation polynomials the way [@ChevillardJoldesLauter2010] does. However, `Sollya` could be used beforehand to feed `aerobus` with appropriate coefficients. `Aerobus` does not provide floating point manipulations (domain normalization) to extend domain of approximation, like it is done in standard library.

## Mathematic definitions

For the sake of completeness, we give basic definitions of the mathematical concepts which the library deals with. However, readers desiring complete and rigorous definitions of the concepts explained below should refer to a mathematical book on algebra, such as [@lang2012algebra] or [@bourbaki2013algebra].

A `ring` $\mathbb{A}$ is a nonempty set with two internal laws, addition and multiplication. There is a neutral element for both, zero and one.
Addition is commutative and associative and every element $x$ has an inverse $-x$. Multiplication is commutative, associative and distributive over addition, meaning that $a(b+c) = ab+ac$ for every $a, b, c$ element. We call it `discrete` if it is countable.

In a `field`, in addition to previous properties, each element (except zero), has an inverse for multiplication.

A `integral domain` is a ring with one additional property. For every element $a, b, c$ such as $ab = ac$, then either $a = 0$ or $b = c$. Such a ring is not always a field, such as $\mathbb{Z}$ shows it.

A `euclidean domain` is an integral domain that can be endowed with a euclidean division.

For such a euclidean domain, we can build two important structures:

### Polynomials $\mathbb{A}[X]$

Polynomials over $\mathbb{A}$ is the free module generated by a base noted $(X^k)_{k\in\mathbb{N}}$. Practically speaking, it's the set of :

$$a_0 + a_1X + \ldots + a_nX^n$$

where $a_n \neq 0$ if $n \neq 0$.

$(a_i)$, the coefficients, are elements of $\mathbb{A}$. The theory states that if $\mathbb{A}$ is a field, then $\mathbb{A}[X]$ is Euclidean. That means notions like division of greatest common divisor (gcd) have a meaning, yielding an arithmetic of polynomials.

### Field of fractions

If $\mathbb{A}$ is Euclidean, we can build its field of fractions: the smallest field containing $\mathbb{A}$.
We construct it as congruences classes of $\mathbb{A}\times \mathbb{A}$ for the relation $(p,q) \sim (pp, qq)\  \mathrm{iff}\ p*qq = q*pp$. Basic algebra shows that this is a field (every element has an inverse). The canonical example is $\mathbb{Q}$, the set of rational numbers.

Given polynomials over a field form an Euclidean ring, we can do the same construction and get rational fractions $P(x) / Q(X)$ where $P$ and $Q$ are polynomials.

### Quotient rings

In an Euclidean domain $\mathbb{A}$, such as $\mathbb{Z}$ or $\mathbb{A}[X]$, we can define the quotient ring of $\mathbb{A}$ by a principal ideal $I$. Given that $I$ is principal, it is generated by an element $X$ and the quotient ring is the ring of rests modulo $X$. When $X$ is `prime` (meaning it has no smallest factors in $\mathbb{A}$), the quotient ring $\mathbb{A}/I$ is a field.

Applied on $\mathbb{Z}$, that operation gives us modular arithmetic and all finite fields of cardinal $q$ where $q$ is a prime number (up to isomorphism). These fields are usually named $\mathbb{Z}/p\mathbb{Z}$. Applied on $\mathbb{Z}/p\mathbb{Z}[X]$, it gives finite Galois fields, meaning all finite fields of cardinal $p^n$ where $p$ is prime (see [@evariste1846memoire]).

## Software

All types of Aerobus have the same structure.

An englobing type describes an algebraic structure. It has a nested type `val` which is always a template model describing elements of the set.

This is because we want to operate on types more than on values. This allows generic implementation, for example of `gcd` (see below) without specifying what are the values.

### Concepts

The library exposes three main `concepts`:

```cpp
template <typename R>
concept IsRing; // see in code or documentation

template <typename R>
concept IsEuclideanDomain; // see in code or documentation

template<typename R>
concept IsField; // see in code or documentation
```

which express the algebraic objects described above. Then, as long as a type satisfies the `IsEuclideanDomain` concept, we can calculate the greatest common divisor of two values of this type using Euclid's algorithm [@heath1956thirteen]. As stated above, this algorithm operates on types instead of values and does not depend on the Ring, making it possible for users to implement another kind of discrete Euclidean domain without worrying about that kind of algorithm.

The same is done for the field of fractions: implementation does not rely on the nature of the underlying Euclidean domain but rather on its structure. It's automatically done by templates, as long as Ring satisfies the appropriate concept.

Doing that way, $\mathbb{Q}$ has the same implementation as rational fractions of polynomials. Users could also get the field of fractions of any ring of their convenience, as long as they implement the required concepts.

### Native types

`Aerobus` exposes several pre-implemented types, as they are common and necessary to do actual computations:

- `i32` and `i64` ($\mathbb{Z}$ seen as 32 bits or 64 bits integers)
- `zpz` the quotient ring $\mathbb{Z}/p\mathbb{Z}$
- `polynomial<T>` where T is a ring
- `FractionField<T>` where T is an Euclidean domain

Polynomial exposes an evaluation function, which automatically generates Horner development and unrolls the loop by generating it at compile time. See [@horner1815new] or [@knuth2014art] for further developments of this method.

Given a polynomial

$$P = \sum_{i=0}^{i ≤ n}a_iX^i = a_0 + a_1X + ... + a_nX^n$$

we can evaluate it by rewriting it this way:

$$P(x) = a_0 + X (a_1 + X (a_2 + X(... + X(a_{n-1} + X an))))$$

which is automaticcaly done by `aerobus` using the `polynomial::val::eval` function.

This evaluation function is `constexpr` and therefore will be completely computed at compile time when called on a constant.

Polynomials also expose Compensated Horner scheme like in [@graillat2006compensated], to gain extra precision when evaluating polynomials close to its roots.

The library also provides built-in integers and functions, such as Bernouilli numbers, factorials or other utilities.

Some well known Taylor series, such as `exp` or `acosh` come preimplemented.

The library comes with a type designed to help the users implement other Taylor series.
If users provide a type `mycoeff` satisfying the appropriate template (depending on the `Ring` of coefficients and degree), the corresponding Taylor expansion can be built automatically as a polynomial over this `Ring` and then, evaluated at some value in a native arithmetic type (such as `double`).

## Misc

### Continued Fractions

`Aerobus` provides [continued fractions](https://en.wikipedia.org/wiki/Continued_fraction), seen as an example of what is possible when you have a proper type representation of the field of fractions.
One can get a rational approximation of numbers using their known representation, given by the On-Line Encyclopedia of Integer Sequences [@OEIS].
Some useful math constants, such as $\pi$ or $e$ are provided preimplemented, from which user can have the corresponding rational number by using (for example) `PI_fraction::type` and a computation with `PI_fraction::val`.

### Known polynomials

There existe many orthogonal polynomial bases used in various domains, from number theory to quantum physics.
`Aerobus` provide predefined implementation for some of them (Laguerre, Hermite, Bernstein, Bessel, ...). These polynomials have integers coefficients by default, but can be defined (specialized) with coefficients in any `Ring` (such as $\mathbb{Z}/2\mathbb{Z}$).

### Quotient rings and Galois fields

If some type meets the `IsRing` concept requirement, Aerobus can generate its quotient ring by a principal ideal generated by some element `X`.

We can then define finite fields such as $\mathbb{Z}/p\mathbb{Z}$ by writing `using Z2Z = Quotient<i32, i32::inject_constant_t<2>>;`.

In $\mathbb{Z}/p\mathbb{Z}[X]$, there are special irreducible polynomials named Conway polynomials [@holt2005handbook], used to build larger finite fields. `Aerobus` exposes Conway polynomials for $p$ smaller than 1000 and degrees smaller than 20.

To speed up compilation for users who don't use them, they are hidden behing the flag `AEROBUS_CONWAY_IMPORTS`. If this is defined, it's possible define $\mathrm{GF}(p, n) = \mathbb{F}_{p^n}$.

For instance, we can compute $\mathbb{F}_4 = \mathrm{GF}(2, 2)$ by writing:

```cpp
using F2 = zpz<2>;
using PF2 = polynomial<F2>;
using F4 = Quotient<PF2, ConwayPolynomial<2, 2>::type>;
```

Multiplication and addition tables are checked to be those of $\mathbb{F}_4$.

Surprisingly, compilation time is not significantly higher when we include `conwaypolynomials.h`. However, we chose to make it optional.

## Acknowledgments

Many thanks to my math teachers, A. Soyeur and M. Gonnord. I also acknowledge indirect contributions from F. Duguet, who showed me the way. I wish also to thank Miss Chloé Gence, who gave me the name of the library.

## Reference
