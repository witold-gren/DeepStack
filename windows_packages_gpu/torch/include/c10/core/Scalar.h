#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <type_traits>

#include <c10/core/ScalarType.h>
#include <c10/macros/Macros.h>
#include <c10/util/Half.h>
#include <c10/util/TypeCast.h>

namespace c10 {

/**
 * Scalar represents a 0-dimensional tensor which contains a single element.
 * Unlike a tensor, numeric literals (in C++) are implicitly convertible to
 * Scalar (which is why, for example, we provide both add(Tensor) and
 * add(Scalar) overloads for many operations). It may also be used in
 * circumstances where you statically know a tensor is 0-dim and single size,
 * but don't know its type.
 */
class C10_API Scalar {
 public:
  Scalar() : Scalar(int64_t(0)) {}

#define DEFINE_IMPLICIT_CTOR(type, name)      \
  Scalar(type vv) : Scalar(vv, true) { }

  AT_FORALL_SCALAR_TYPES_AND2(Half, BFloat16, DEFINE_IMPLICIT_CTOR)
  AT_FORALL_COMPLEX_TYPES(DEFINE_IMPLICIT_CTOR)
  // TODO: remove the std::complex below
  DEFINE_IMPLICIT_CTOR(std::complex<float>, x)
  DEFINE_IMPLICIT_CTOR(std::complex<double>, x)

#undef DEFINE_IMPLICIT_CTOR

  // Value* is both implicitly convertible to SymbolicVariable and bool which
  // causes ambiguosity error. Specialized constructor for bool resolves this
  // problem.
  template <
      typename T,
      typename std::enable_if<std::is_same<T, bool>::value, bool>::type* =
          nullptr>
  Scalar(T vv) : tag(Tag::HAS_b) {
    v.i = convert<int64_t, bool>(vv);
  }

#define DEFINE_ACCESSOR(type, name)                       \
  type to##name() const {                                 \
    if (Tag::HAS_d == tag) {                              \
      return checked_convert<type, double>(v.d, #type);   \
    } else if (Tag::HAS_z == tag) {                       \
      return checked_convert<type, c10::complex<double>>( \
          v.z, #type);                                    \
    } if (Tag::HAS_b == tag) {                            \
      return checked_convert<type, bool>(v.i, #type);     \
    } else {                                              \
      return checked_convert<type, int64_t>(v.i, #type);  \
    }                                                     \
  }

  // TODO: Support ComplexHalf accessor
  AT_FORALL_SCALAR_TYPES_WITH_COMPLEX_EXCEPT_COMPLEX_HALF(DEFINE_ACCESSOR)

  // also support scalar.to<int64_t>();
  template <typename T>
  T to() const;

#undef DEFINE_ACCESSOR
  bool isFloatingPoint() const {
    return Tag::HAS_d == tag;
  }

  C10_DEPRECATED_MESSAGE("isIntegral is deprecated. Please use the overload with 'includeBool' parameter instead.")
  bool isIntegral() const {
    return Tag::HAS_i == tag;
  }
  bool isIntegral(bool includeBool) const {
    return Tag::HAS_i == tag || (includeBool && isBoolean());
  }

  bool isComplex() const {
    return Tag::HAS_z == tag;
  }
  bool isBoolean() const {
    return Tag::HAS_b == tag;
  }

  Scalar operator-() const;

  ScalarType type() const {
    if (isComplex()) {
      return ScalarType::ComplexDouble;
    } else if (isFloatingPoint()) {
      return ScalarType::Double;
    } else if (isIntegral(/*includeBool=*/false)) {
      return ScalarType::Long;
    } else if (isBoolean()) {
      return ScalarType::Bool;
    } else {
      throw std::runtime_error("Unknown scalar type.");
    }
  }

 private:
    template<typename T,
             typename std::enable_if<std::is_integral<T>::value && ! std::is_same<T, bool>::value, bool>::type* =
                 nullptr>
    Scalar(T vv, bool) : tag(Tag::HAS_i) {
      v.i = convert<decltype(v.i), T>(vv);
    }

    template<typename T,
             typename std::enable_if<!std::is_integral<T>::value && !c10::is_complex_t<T>::value, bool>::type* =
                 nullptr>
    Scalar(T vv, bool) : tag(Tag::HAS_d) {
      v.d = convert<decltype(v.d), T>(vv);
    }

    template<typename T,
             typename std::enable_if<c10::is_complex_t<T>::value, bool>::type* =
                 nullptr>
    Scalar(T vv, bool) : tag(Tag::HAS_z) {
      v.z = convert<decltype(v.z), T>(vv);
    }

  // We can't set v in the initializer list using the
  // syntax v{ .member = ... } because it doesn't work on MSVC

  enum class Tag { HAS_d, HAS_i, HAS_z, HAS_b };
  Tag tag;
  union v_t {
    double d;
    int64_t i;
    c10::complex<double> z;
    v_t(){}  // default constructor
  } v;
};

// define the scalar.to<int64_t>() specializations
template <typename T>
inline T Scalar::to() const {
  throw std::runtime_error("to() cast to unexpected type.");
}

#define DEFINE_TO(T, name)    \
  template <>                 \
  inline T Scalar::to<T>() const {  \
    return to##name();        \
  }
AT_FORALL_SCALAR_TYPES_WITH_COMPLEX_EXCEPT_COMPLEX_HALF(DEFINE_TO)
#undef DEFINE_TO

// TODO(@zasdfgbnm): Remove this!
// This is needed only when the migration of std::complex to c10::complex
// is not done. This should be removed once the migration is done.
template <>
inline std::complex<float> Scalar::to() const {
  return static_cast<std::complex<float>>(toComplexFloat());
}
template <>
inline std::complex<double> Scalar::to() const {
  return static_cast<std::complex<double>>(toComplexDouble());
}
// end TODO

} // namespace c10
