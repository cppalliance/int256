// Copyright 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/int256/int256.hpp>

#if defined(NDEBUG) && !defined(UBSAN) && !defined(ASAN) && !defined(__SANITIZE_ADDRESS__) && !defined(BOOST_INT256_DISABLE_EXCEPTIONS)
#define BOOST_INT256_BENCHMARK_I256
#endif // NDEBUG

// GCC-5 support breaks down with errors propagating from Boost.Multiprecision
#if defined(BOOST_INT256_BENCHMARK_I256) && defined(__GNUC__) && __GNUC__ == 5
#  undef BOOST_INT256_BENCHMARK_I256
#endif

#include <iostream>

#ifdef BOOST_INT256_BENCHMARK_I256

#include <boost/int256/numeric.hpp>
#include "benchmark_results.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <random>
#include <string>
#include <vector>

// Element and repetition counts. They start at the defaults of the shared
// options and main() refreshes them from --elements and --repetitions.
std::size_t N {bench::config().elements};
unsigned K {bench::config().repetitions};

#ifdef __clang__
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wfloat-equal"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#  pragma clang diagnostic ignored "-Wdouble-promotion"
#  pragma clang diagnostic ignored "-Wsign-conversion"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#  pragma clang diagnostic ignored "-Wundef"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wunused-variable"
#  if defined(__has_warning)
#    if __has_warning("-Wpragma-clang-attribute")
#      pragma clang diagnostic ignored "-Wpragma-clang-attribute"
#    endif
#  endif
#  if __clang_major__ >= 20
#    pragma clang diagnostic ignored "-Wdeprecated-missing-comma-variadic-parameter"
#    pragma clang diagnostic ignored "-Wfortify-source"
#  endif
#  define BOOST_INT256_NO_INLINE __attribute__ ((__noinline__))
#elif defined(_MSC_VER)
#  define BOOST_INT256_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#  pragma GCC diagnostic ignored "-Wdouble-promotion"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wcast-qual"
#  define BOOST_INT256_NO_INLINE __attribute__ ((__noinline__))
#  if __GNUC__ >= 11
#    pragma GCC diagnostic ignored "-Wstringop-overread"
#  endif
#  if __GNUC__ > 6
#    pragma GCC diagnostic ignored "-Wduplicated-branches"
#  endif
#endif

#include <boost/multiprecision/cpp_int.hpp>

// The second comparator is optional: it is measured when the wide-integer headers
// happen to be on the include path (the fuzzing jobs check them out anyway).
// There is no separate "intwide_t" name in that library: the same uintwide_t
// template takes a fourth, defaulted IsSigned template parameter, exactly as
// test/fuzzing/test_fuzzing_div_signed_versus_wide_int.cpp uses it.
#if __has_include(<math/wide_integer/uintwide_t.h>)
#  include <math/wide_integer/uintwide_t.h>
#  define BOOST_INT256_BENCHMARK_WIDE_INTEGER
#endif

using boost::int256::int256;
using boost::int256::uint256;
using mp_i256 = boost::multiprecision::int256_t;

#ifdef BOOST_INT256_BENCHMARK_WIDE_INTEGER
using wide_i256 = ::math::wide_integer::uintwide_t<256U, std::uint32_t, void, true>;
#endif

// mp::int256_t is sign-magnitude with range +-(2^256 - 1), the portability
// baseline documented in comp_to_multiprecision.adoc, not a two's-complement
// oracle. It is still the reference implementation here because no platform
// has a hardware 256-bit type.
using baseline_type = mp_i256;

// Names of the implementations under test. The generated documentation tables and
// the plot legends use these labels verbatim, so each one spells the type the way
// a user would write it.
template <typename T>
const char* impl_label() noexcept;

template <>
const char* impl_label<int256>() noexcept
{
    return "int256";
}

template <>
const char* impl_label<mp_i256>() noexcept
{
    return "boost::mp::int256";
}

#ifdef BOOST_INT256_BENCHMARK_WIDE_INTEGER

template <>
const char* impl_label<wide_i256>() noexcept
{
    return "intwide_t<256>";
}

#endif

//=====================================
// Conversions between the three types
//=====================================

// Builds a value of the type under test from four words, most significant first,
// interpreted as two's complement (so a set top bit produces a negative value).
template <typename T>
T from_words(const std::uint64_t w3, const std::uint64_t w2, const std::uint64_t w1, const std::uint64_t w0);

template <>
int256 from_words(const std::uint64_t w3, const std::uint64_t w2, const std::uint64_t w1, const std::uint64_t w0)
{
    return int256(static_cast<std::int64_t>(w3), w2, w1, w0);
}

// mp::int256_t is sign-magnitude and its arithmetic is checked, so 2^256 itself
// cannot be formed inside it (its magnitude tops out at 2^256 - 1). The two's
// complement adjustment is therefore built in a wide (512-bit) signed
// intermediate first, exactly as the test oracle's to_smp does, and only the
// final, always-in-range value is narrowed down to mp_i256.
template <>
mp_i256 from_words(const std::uint64_t w3, const std::uint64_t w2, const std::uint64_t w1, const std::uint64_t w0)
{
    boost::multiprecision::int512_t wide {w3};
    wide <<= 64U;
    wide |= w2;
    wide <<= 64U;
    wide |= w1;
    wide <<= 64U;
    wide |= w0;

    if (w3 >= (UINT64_C(1) << 63))
    {
        wide -= boost::multiprecision::int512_t{1} << 256;
    }

    return static_cast<mp_i256>(wide);
}

#ifdef BOOST_INT256_BENCHMARK_WIDE_INTEGER

// intwide_t is a fixed-width two's-complement type, like the library's own
// int256, so the top word can be handed to it as std::int64_t directly and the
// lower words folded in with ordinary shift-and-or.
template <>
wide_i256 from_words(const std::uint64_t w3, const std::uint64_t w2, const std::uint64_t w1, const std::uint64_t w0)
{
    wide_i256 value {static_cast<std::int64_t>(w3)};
    value <<= 64U;
    value |= wide_i256{w2};
    value <<= 64U;
    value |= wide_i256{w1};
    value <<= 64U;
    value |= wide_i256{w0};
    return value;
}

#endif

// The single 64-bit sink value taken out of a result, so that a pass cannot be
// optimized away without paying for a full conversion
template <typename T>
std::uint64_t to_u64(const T& value);

template <>
std::uint64_t to_u64(const int256& value)
{
    return value.words[0];
}

template <>
std::uint64_t to_u64(const mp_i256& value)
{
    // abs() sidesteps the (backend-defined) behavior of bitwise/modulo
    // operations on a negative sign-magnitude value; the low 64 bits of the
    // magnitude are enough for a benchmark sink.
    return static_cast<std::uint64_t>(boost::multiprecision::abs(value) % (mp_i256{1} << 64));
}

#ifdef BOOST_INT256_BENCHMARK_WIDE_INTEGER

template <>
std::uint64_t to_u64(const wide_i256& value)
{
    return static_cast<std::uint64_t>(value & wide_i256{UINT64_MAX});
}

#endif

//=====================================
// Data
//=====================================

// The operand width groups. random_width is the one the documentation publishes;
// the fixed widths are there to show where the division tiers change cost.
// Mixed signs arise naturally: whenever the top significant word is drawn, its
// high bit is set about half the time, which is what makes int256 different
// from Boost.Int128's signed benchmark (non-negative operands only).
enum class shape
{
    four_word,
    one_word,
    two_word,
    alternating,
    random_width
};

const char* shape_name(const shape s) noexcept
{
    switch (s)
    {
        case shape::four_word:
            return "four_word";
        case shape::one_word:
            return "one_word";
        case shape::two_word:
            return "two_word";
        case shape::alternating:
            return "alternating";
        case shape::random_width:
            return "random_width";
    }

    return "unknown";
}

// Every implementation is filled from the same seed, so the three vectors hold
// the same values and the timings are comparable element by element.
template <typename T>
std::vector<T> generate_random_vector(const shape s, const std::size_t size = N, const unsigned seed = 42U)
{
    std::mt19937_64 gen {seed};
    std::uniform_int_distribution<std::uint64_t> word_dist {UINT64_C(0), UINT64_MAX};
    std::uniform_int_distribution<int> width_dist {1, 4};

    std::vector<T> result(size);

    for (std::size_t i {}; i < size; ++i)
    {
        auto words = 4;

        switch (s)
        {
            case shape::four_word:
                words = 4;
                break;
            case shape::one_word:
                words = 1;
                break;
            case shape::two_word:
                words = 2;
                break;
            case shape::alternating:
                words = (i % 2U == 0U) ? 4 : 1;
                break;
            case shape::random_width:
                words = width_dist(gen);
                break;
        }

        const auto w0 = word_dist(gen);
        const auto w1 = words > 1 ? word_dist(gen) : UINT64_C(0);
        const auto w2 = words > 2 ? word_dist(gen) : UINT64_C(0);
        const auto w3 = words > 3 ? word_dist(gen) : UINT64_C(0);

        result[i] = from_words<T>(w3, w2, w1, w0);
    }

    return result;
}

std::vector<int> generate_shift_vector(const std::size_t size = N, const unsigned seed = 43U)
{
    std::mt19937_64 gen {seed};
    std::uniform_int_distribution<int> dist {0, 255};

    std::vector<int> result(size);
    for (std::size_t i {}; i < size; ++i)
    {
        result[i] = dist(gen);
    }

    return result;
}

//=====================================
// Timing
//=====================================

using clock_type = std::chrono::steady_clock;

// Records one timing in the data set and prints it as it is measured
void report(const char* operation, const char* label, const clock_type::time_point start,
            const clock_type::time_point stop, const std::uint64_t sink)
{
    const auto us = bench::elapsed_us(start, stop);
    bench::record(operation, label, us);

    std::cerr << std::left << std::setw(12) << operation << '<' << std::setw(19) << label << ">: "
              << std::right << std::setw(10) << us << " us (s=" << sink << ")\n";
}

// The streaming shape: each pass writes a whole vector of results and then reads
// one element of it into the sink. Nothing is measured per element, so no
// DoNotOptimize barrier sits inside the loop to distort the codegen.
template <typename T, typename Op>
BOOST_INT256_NO_INLINE void stream_binary(const std::vector<T>& in, std::vector<T>& out, Op op, const char* operation)
{
    const auto pairs = in.size() - 1U;
    std::uint64_t sink {0};

    const auto start = clock_type::now();

    for (unsigned k {}; k < K; ++k)
    {
        for (std::size_t i {}; i < pairs; ++i)
        {
            out[i] = op(in[i], in[i + 1U]);
        }

        sink += to_u64(out[k % pairs]);
    }

    const auto stop = clock_type::now();
    report(operation, impl_label<T>(), start, stop, sink);
}

// The shift form, where the count comes from its own vector
template <typename T, typename Op>
BOOST_INT256_NO_INLINE void stream_shift(const std::vector<T>& in, const std::vector<int>& counts,
                                         std::vector<T>& out, Op op, const char* operation)
{
    std::uint64_t sink {0};

    const auto start = clock_type::now();

    for (unsigned k {}; k < K; ++k)
    {
        for (std::size_t i {}; i < in.size(); ++i)
        {
            out[i] = op(in[i], counts[i]);
        }

        sink += to_u64(out[k % in.size()]);
    }

    const auto stop = clock_type::now();
    report(operation, impl_label<T>(), start, stop, sink);
}

// A comparison returns a bool, so the natural sink is the count of true results
template <typename T, typename Op>
BOOST_INT256_NO_INLINE std::int64_t stream_compare(const std::vector<T>& in, Op op, const char* operation)
{
    const auto pairs = in.size() - 1U;
    std::uint64_t sink {0};

    const auto start = clock_type::now();

    for (unsigned k {}; k < K; ++k)
    {
        for (std::size_t i {}; i < pairs; ++i)
        {
            sink += static_cast<std::uint64_t>(op(in[i], in[i + 1U]));
        }
    }

    const auto stop = clock_type::now();
    report(operation, impl_label<T>(), start, stop, sink);
    return bench::elapsed_us(start, stop);
}

//=====================================
// The operations
//
// Scope is deliberately narrower than the unsigned driver: the sign-agnostic
// bitwise operators (and, or, xor, not) are already proven within +-1
// instruction of uint256 in the header performance rules and are not
// re-measured here. This covers only the rows that can differ for a signed
// type: comparisons, add/sub/mul, division and remainder (sign of quotient and
// remainder), and both shifts (right shift is arithmetic for int256).
//=====================================

template <typename T>
void run_comparisons(const std::vector<T>& in)
{
    std::int64_t total {0};

    total += stream_compare(in, [](const T& a, const T& b) { return a == b; }, "eq");
    total += stream_compare(in, [](const T& a, const T& b) { return a != b; }, "ne");
    total += stream_compare(in, [](const T& a, const T& b) { return a < b; }, "lt");
    total += stream_compare(in, [](const T& a, const T& b) { return a <= b; }, "le");
    total += stream_compare(in, [](const T& a, const T& b) { return a > b; }, "gt");
    total += stream_compare(in, [](const T& a, const T& b) { return a >= b; }, "ge");

    // The comparison row of the documentation tables is the total of all six
    bench::record("comparisons", impl_label<T>(), total);
    std::cerr << std::left << std::setw(12) << "comparisons" << '<' << std::setw(19) << impl_label<T>()
              << ">: " << std::right << std::setw(10) << total << " us\n";
}

template <typename T>
void run_arithmetic(const std::vector<T>& in, std::vector<T>& out)
{
    stream_binary(in, out, [](const T& a, const T& b) { return a + b; }, "add");
    stream_binary(in, out, [](const T& a, const T& b) { return a - b; }, "sub");
    stream_binary(in, out, [](const T& a, const T& b) { return a * b; }, "mul");

    // The divisor is forced odd so that it is never zero; forcing the low bit
    // is sign independent, so this works whether b is negative or not
    stream_binary(in, out, [](const T& a, const T& b) { return a / (b | T{1U}); }, "div");
    stream_binary(in, out, [](const T& a, const T& b) { return a % (b | T{1U}); }, "mod");

    // The narrow divisors take the one-word and half-word tiers instead. They
    // are built from a non-negative magnitude only, so only the dividend's sign
    // varies in these two rows.
    stream_binary(in, out, [](const T& a, const T& b)
    {
        return a / from_words<T>(0U, 0U, 0U, to_u64(b) | UINT64_C(1));
    }, "div64");

    stream_binary(in, out, [](const T& a, const T& b)
    {
        return a / from_words<T>(0U, 0U, 0U, static_cast<std::uint32_t>(to_u64(b)) | UINT32_C(1));
    }, "div32");
}

template <typename T>
void run_shifts(const std::vector<T>& in, const std::vector<int>& counts, std::vector<T>& out)
{
    stream_shift(in, counts, out, [](const T& a, const int s) { return a << s; }, "shl");
    stream_shift(in, counts, out, [](const T& a, const int s) { return a >> s; }, "shr");
}

// One implementation over one operand shape. The vectors are built and released
// inside this function so that only one implementation is resident at a time,
// which matters at ten million 256-bit elements.
template <typename T>
void run_implementation(const shape s, const std::vector<int>& counts)
{
    const auto in {generate_random_vector<T>(s)};
    std::vector<T> out(in.size());

    run_comparisons(in);
    run_arithmetic(in, out);
    run_shifts(in, counts, out);

    std::cerr << std::endl;
}

int main(const int argc, char* argv[])
{
    bench::parse_options(argc, argv);

    // A 32-bit address space cannot hold several vectors of the default length,
    // so the count is capped there. The data set records what was measured.
    if (sizeof(void*) == 4U && bench::config().elements > 5000000U)
    {
        bench::config().elements = 5000000U;
        std::cerr << "32-bit platform: capping --elements at 5000000" << std::endl;
    }

    N = bench::config().elements;
    K = bench::config().repetitions;

    std::cerr << "Running with " << N << " elements and " << K << " repetitions per operation\n" << std::endl;

    const auto counts {generate_shift_vector()};

    const shape shapes[] {shape::four_word, shape::one_word, shape::two_word,
                          shape::alternating, shape::random_width};

    for (const auto s : shapes)
    {
        bench::set_group(shape_name(s));
        std::cerr << "==================== " << shape_name(s) << " ====================" << std::endl;

        run_implementation<int256>(s, counts);
        run_implementation<mp_i256>(s, counts);

        #ifdef BOOST_INT256_BENCHMARK_WIDE_INTEGER
        run_implementation<wide_i256>(s, counts);
        #endif
    }

    if (!bench::write_json(bench::metadata{"int256", "i256", impl_label<baseline_type>()}))
    {
        return 2;
    }

    // The Jamfile declares this target with run-fail, so a successful run reports 1.
    return 1;
}

#else // No benchmarks

int main()
{
    std::cerr << "Benchmarks Not Run" << std::endl;
    return 1;
}

#endif
