#pragma once

#include <cstdint>
#include <iostream>
#include <type_traits>
#include <compare>

struct FixedTag {};
struct FastTag {};
struct RawTag {}; 

template <size_t N, size_t K, typename Tag = FixedTag>
struct FixedPoint {
    static_assert(N <= 64, "FixedPoint<N, K> поддерживает максимум 64 бита.");
    static_assert(N > 0, "FixedPoint<N, K> должно содержать хотя бы 1 бит.");
    static_assert(K < N, "Количество битов дробной части должно быть меньше общего количества битов.");

    using StorageType = std::conditional_t<
        std::is_same_v<Tag, FixedTag>,
        std::conditional_t<
            (N <= 8), int8_t,
            std::conditional_t<
                (N <= 16), int16_t,
                std::conditional_t<
                    (N <= 32), int32_t,
                    int64_t
                >
            >
        >,
        std::conditional_t<
            (N <= 8), int_fast8_t,
            std::conditional_t<
                (N <= 16), int_fast16_t,
                std::conditional_t<
                    (N <= 32), int_fast32_t,
                    int_fast64_t
                >
            >
        >
    >;

    StorageType v;

    FixedPoint() : v(0) {}

        friend std::ostream& operator<<(std::ostream& os, const FixedPoint& fp) {
        os << static_cast<double>(fp);
        return os;
        }

    constexpr FixedPoint(int32_t value) 
        : v(static_cast<StorageType>(value) << K) {}

    constexpr FixedPoint(float f) 
        : v(static_cast<StorageType>(f * (1ULL << K))) {}

    constexpr FixedPoint(double f) 
        : v(static_cast<StorageType>(f * (1ULL << K))) {}

    constexpr FixedPoint(RawTag, StorageType raw) 
        : v(raw) {}

    template <size_t M, size_t L, typename OtherTag>
    constexpr FixedPoint(const FixedPoint<M, L, OtherTag>& other) {
        double asDouble = static_cast<double>(other);
        v = static_cast<StorageType>(asDouble * (1ULL << K));
    }

    template <size_t M, size_t L, typename OtherTag>
    FixedPoint& operator=(const FixedPoint<M, L, OtherTag>& other) {
        double asDouble = static_cast<double>(other);
        v = static_cast<StorageType>(asDouble * (1ULL << K));
        return *this;
    }

public:
    static constexpr FixedPoint from_raw(StorageType raw) {
        return FixedPoint(RawTag{}, raw);
    }

    explicit operator float() const {
        return static_cast<float>(v) / (1ULL << K);
    }

    explicit operator double() const {
        return static_cast<double>(v) / (1ULL << K);
    }

    template <typename Integral, typename = std::enable_if_t<std::is_integral_v<Integral>>>
    operator Integral() const {
        return static_cast<Integral>(v >> K);
    }

    auto operator<=>(const FixedPoint&) const = default;
    bool operator==(const FixedPoint&) const = default;
    FixedPoint& operator=(const FixedPoint& other) = default;

    FixedPoint operator+(const FixedPoint& other) const {
        FixedPoint result;
        result.v = this->v + other.v;
        return result;
    }

    FixedPoint operator-(const FixedPoint& other) const {
        FixedPoint result;
        result.v = this->v - other.v;
        return result;
    }

    FixedPoint operator*(const FixedPoint& other) const {
        using IntermediateType = std::conditional_t<
            (N <= 16), int_fast32_t,
            int_fast64_t
        >;
        IntermediateType temp = static_cast<IntermediateType>(v) * static_cast<IntermediateType>(other.v);
        FixedPoint result;
        result.v = static_cast<StorageType>(temp >> K);
        return result;
    }

    FixedPoint operator/(const FixedPoint& other) const {
        using IntermediateType = std::conditional_t<
            (N <= 16), int_fast32_t,
            int_fast64_t
        >;
        IntermediateType temp = (static_cast<IntermediateType>(v) << K) / other.v;
        FixedPoint result;
        result.v = static_cast<StorageType>(temp);
        return result;
    }

    FixedPoint& operator+=(const FixedPoint& other) {
        this->v += other.v;
        return *this;
    }

    FixedPoint& operator-=(const FixedPoint& other) {
        this->v -= other.v;
        return *this;
    }

    FixedPoint& operator*=(const FixedPoint& other) {
        *this = *this * other;
        return *this;
    }

    FixedPoint& operator/=(const FixedPoint& other) {
        *this = *this / other;
        return *this;
    }
};

template <size_t N, size_t K>
using FIXED = FixedPoint<N, K, FixedTag>;

template <size_t N, size_t K>
using FAST_FIXED = FixedPoint<N, K, FastTag>;

template <size_t N1, size_t K1, typename Tag1, size_t N2, size_t K2, typename Tag2>
constexpr bool operator<=(const FixedPoint<N1, K1, Tag1>& lhs, const FixedPoint<N2, K2, Tag2>& rhs) {
    return static_cast<double>(lhs) <= static_cast<double>(rhs);
}

template <size_t N1, size_t K1, typename Tag1, size_t N2, size_t K2, typename Tag2>
constexpr bool operator<(const FixedPoint<N1, K1, Tag1>& lhs, const FixedPoint<N2, K2, Tag2>& rhs) {
    return static_cast<double>(lhs) < static_cast<double>(rhs);
}

template <size_t N1, size_t K1, typename Tag1, size_t N2, size_t K2, typename Tag2>
constexpr bool operator>=(const FixedPoint<N1, K1, Tag1>& lhs, const FixedPoint<N2, K2, Tag2>& rhs) {
    return static_cast<double>(lhs) >= static_cast<double>(rhs);
}

template <size_t N1, size_t K1, typename Tag1, size_t N2, size_t K2, typename Tag2>
constexpr bool operator>(const FixedPoint<N1, K1, Tag1>& lhs, const FixedPoint<N2, K2, Tag2>& rhs) {
    return static_cast<double>(lhs) > static_cast<double>(rhs);
}

template <size_t N1, size_t K1, typename Tag1, size_t N2, size_t K2, typename Tag2>
constexpr bool operator==(const FixedPoint<N1, K1, Tag1>& lhs, const FixedPoint<N2, K2, Tag2>& rhs) {
    if constexpr (K1 > K2) {
        return lhs.v == (rhs.v << (K1 - K2));
    } else if constexpr (K2 > K1) {
        return (lhs.v << (K2 - K1)) == rhs.v;
    } else {
        return lhs.v == rhs.v;
    }
}

template <size_t N1, size_t K1, typename Tag1, size_t N2, size_t K2, typename Tag2>
constexpr bool operator!=(const FixedPoint<N1, K1, Tag1>& lhs, const FixedPoint<N2, K2, Tag2>& rhs) {
    return !(lhs == rhs);
}

template <size_t N1, size_t K1, typename Tag1, size_t N2, size_t K2, typename Tag2>
constexpr FixedPoint<(N1 > N2 ? N1 : N2), (K1 > K2 ? K1 : K2)>
operator+(const FixedPoint<N1, K1, Tag1>& lhs, const FixedPoint<N2, K2, Tag2>& rhs) {
    double sum = static_cast<double>(lhs) + static_cast<double>(rhs);
    constexpr size_t new_N = (N1 > N2 ? N1 : N2);
    constexpr size_t new_K = (K1 > K2 ? K1 : K2);
    return FixedPoint<new_N, new_K>(sum);
}

template <size_t N1, size_t K1, typename Tag1, size_t N2, size_t K2, typename Tag2>
constexpr std::enable_if_t<!(N1 == N2 && K1 == K2 && std::is_same_v<Tag1, Tag2>), FixedPoint<(N1 > N2 ? N1 : N2), (K1 > K2 ? K1 : K2)>>
operator-(const FixedPoint<N1, K1, Tag1>& lhs, const FixedPoint<N2, K2, Tag2>& rhs) {
    double diff = static_cast<double>(lhs) - static_cast<double>(rhs);
    constexpr size_t new_N = (N1 > N2 ? N1 : N2);
    constexpr size_t new_K = (K1 > K2 ? K1 : K2);
    return FixedPoint<new_N, new_K>(diff);
}

template <size_t N1, size_t K1, typename Tag1, size_t N2, size_t K2, typename Tag2>
constexpr std::enable_if_t<!(N1 == N2 && K1 == K2 && std::is_same_v<Tag1, Tag2>), FixedPoint<(N1 > N2 ? N1 : N2), (K1 > K2 ? K1 : K2)>>
operator*(const FixedPoint<N1, K1, Tag1>& lhs, const FixedPoint<N2, K2, Tag2>& rhs) {
    double product = static_cast<double>(lhs) * static_cast<double>(rhs);
    constexpr size_t new_N = (N1 > N2 ? N1 : N2);
    constexpr size_t new_K = (K1 > K2 ? K1 : K2);
    return FixedPoint<new_N, new_K>(product);
}

template <size_t N1, size_t K1, typename Tag1, size_t N2, size_t K2, typename Tag2>
constexpr FixedPoint<(N1 > N2 ? N1 : N2), (K1 > K2 ? K1 : K2)>
operator/(const FixedPoint<N1, K1, Tag1>& lhs, const FixedPoint<N2, K2, Tag2>& rhs) {
    double quotient = static_cast<double>(lhs) / static_cast<double>(rhs);
    constexpr size_t new_N = (N1 > N2 ? N1 : N2);
    constexpr size_t new_K = (K1 > K2 ? K1 : K2);
    return FixedPoint<new_N, new_K>(quotient);
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator+(double lhs, const FixedPoint<N, K, Tag>& rhs) {
    return FixedPoint<N, K, Tag>(lhs + static_cast<double>(rhs));
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator+(const FixedPoint<N, K, Tag>& lhs, float rhs) {
    return FixedPoint<N, K, Tag>(static_cast<float>(lhs) + rhs);
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator+(float lhs, const FixedPoint<N, K, Tag>& rhs) {
    return FixedPoint<N, K, Tag>(lhs + static_cast<float>(rhs));
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator+(const FixedPoint<N, K, Tag>& lhs, int rhs) {
    return lhs + FixedPoint<N, K, Tag>(rhs);
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator+(int lhs, const FixedPoint<N, K, Tag>& rhs) {
    return FixedPoint<N, K, Tag>(lhs) + rhs;
}

template <size_t N, size_t K, typename Tag>
float operator+=(float& lhs, const FixedPoint<N, K, Tag>& rhs) {
    lhs += static_cast<float>(rhs);
    return lhs;
}

template<size_t N, size_t K, typename Tag>
double operator+=(double& lhs, const FixedPoint<N, K, Tag>& rhs) {
    lhs += static_cast<double>(rhs);
    return lhs;
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator-(double lhs, const FixedPoint<N, K, Tag>& rhs) {
    return FixedPoint<N, K, Tag>(lhs - static_cast<double>(rhs));
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator-(const FixedPoint<N, K, Tag>& lhs, float rhs) {
    return FixedPoint<N, K, Tag>(static_cast<float>(lhs) - rhs);
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator-(float lhs, const FixedPoint<N, K, Tag>& rhs) {
    return FixedPoint<N, K, Tag>(lhs - static_cast<float>(rhs));
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator-(const FixedPoint<N, K, Tag>& lhs, int rhs) {
    return lhs - FixedPoint<N, K, Tag>(rhs);
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator-(int lhs, const FixedPoint<N, K, Tag>& rhs) {
    return FixedPoint<N, K, Tag>(lhs) - rhs;
}

template <size_t N, size_t K, typename Tag>
double operator-(double lhs, const FixedPoint<N, K, Tag>& rhs) {
    return lhs - static_cast<double>(rhs);
}

template<size_t N, size_t K, typename Tag>
double operator-=(double& lhs, const FixedPoint<N, K, Tag>& rhs) {
    lhs -= static_cast<float>(rhs);
    return lhs;
}

template <size_t N, size_t K, typename Tag>
float operator-=(float& lhs, const FixedPoint<N, K, Tag>& rhs) {
    lhs -= static_cast<float>(rhs);
    return lhs;
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator*(double lhs, const FixedPoint<N, K, Tag>& rhs) {
    return FixedPoint<N, K, Tag>(lhs * static_cast<double>(rhs));
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator*(const FixedPoint<N, K, Tag>& lhs, double rhs) {
    return FixedPoint<N, K, Tag>(static_cast<double>(lhs) * rhs);
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator*(const FixedPoint<N, K, Tag>& lhs, float rhs) {
    return FixedPoint<N, K, Tag>(static_cast<float>(lhs) * rhs);
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator*(float lhs, const FixedPoint<N, K, Tag>& rhs) {
    return FixedPoint<N, K, Tag>(lhs * static_cast<float>(rhs));
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator*(const FixedPoint<N, K, Tag>& lhs, int rhs) {
    return lhs * FixedPoint<N, K, Tag>(rhs);
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator*(int lhs, const FixedPoint<N, K, Tag>& rhs) {
    return FixedPoint<N, K, Tag>(lhs) * rhs;
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator/(const FixedPoint<N, K, Tag>& lhs, double rhs) {
    double result = static_cast<double>(lhs) / rhs;
    return FixedPoint<N, K, Tag>(result);
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator/(double lhs, const FixedPoint<N, K, Tag>& rhs) {
    double result = lhs / static_cast<double>(rhs);
    return FixedPoint<N, K, Tag>(result);
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator/(const FixedPoint<N, K, Tag>& lhs, float rhs) {
    return FixedPoint<N, K, Tag>(static_cast<float>(lhs) / rhs);
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator/(float lhs, const FixedPoint<N, K, Tag>& rhs) {
    return FixedPoint<N, K, Tag>(lhs / static_cast<float>(rhs));
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator/(const FixedPoint<N, K, Tag>& lhs, int rhs) {
    return FixedPoint<N, K, Tag>(static_cast<double>(lhs) / rhs);
}

template <size_t N, size_t K, typename Tag>
FixedPoint<N, K, Tag> operator/(int lhs, const FixedPoint<N, K, Tag>& rhs) {
    return FixedPoint<N, K, Tag>(lhs / static_cast<double>(rhs));
}

template <size_t N, size_t K, typename Tag>
double operator/(const FixedPoint<N, K, Tag>& lhs, double rhs) {
    return static_cast<double>(lhs) / rhs;
}

template <size_t N, size_t K, typename Tag>
double operator/(double lhs, const FixedPoint<N, K, Tag>& rhs) {
    return lhs / static_cast<double>(rhs);
}

template <size_t N, size_t K, typename Tag>
inline bool operator==(const FixedPoint<N, K, Tag>& lhs, int rhs) {
    return static_cast<double>(lhs) == rhs;
}

template <size_t N, size_t K, typename Tag>
inline bool operator==(const FixedPoint<N, K, Tag>& lhs, float rhs) {
    return static_cast<double>(lhs) == rhs;
}

template <size_t N, size_t K, typename Tag>
inline bool operator==(const FixedPoint<N, K, Tag>& lhs, double rhs) {
    return static_cast<double>(lhs) == rhs;
}

template <size_t N, size_t K, typename Tag>
inline bool operator==(double lhs, const FixedPoint<N, K, Tag>& rhs) {
    return lhs == static_cast<double>(rhs);
}

template <size_t N, size_t K, typename Tag>
bool operator<=(const FixedPoint<N, K>& lhs, double rhs) {
    return static_cast<double>(lhs) <= rhs;
}

template <size_t N, size_t K, typename Tag>
bool operator<=(double lhs, const FixedPoint<N, K>& rhs) {
    return lhs <= static_cast<double>(rhs);
}

template <size_t N, size_t K, typename Tag>
inline bool operator<=(const FixedPoint<N, K, Tag>& lhs, double rhs) {
    return static_cast<double>(lhs) <= rhs;
}

template <size_t N, size_t K, typename Tag>
inline bool operator<=(const FixedPoint<N, K, Tag>& lhs, float rhs) {
    return static_cast<double>(lhs) <= rhs;
}

template <size_t N, size_t K, typename Tag>
inline bool operator<=(const FixedPoint<N, K, Tag>& lhs, int rhs) {
    return static_cast<double>(lhs) <= rhs;
}

template <size_t N, size_t K, typename Tag>
inline bool operator<=(double lhs, const FixedPoint<N, K, Tag>& rhs) {
    return lhs <= static_cast<double>(rhs);
}

template <size_t N, size_t K, typename Tag>
inline bool operator<=(float lhs, const FixedPoint<N, K, Tag>& rhs) {
    return lhs <= static_cast<double>(rhs);
}

template <size_t N, size_t K, typename Tag>
bool operator<(const FixedPoint<N, K>& lhs, double rhs) {
    return static_cast<double>(lhs) < rhs;
}

template <size_t N, size_t K, typename Tag>
bool operator<(double lhs, const FixedPoint<N, K>& rhs) {
    return lhs < static_cast<double>(rhs);
}

template <size_t N, size_t K, typename Tag>
inline bool operator<(const FixedPoint<N, K, Tag>& lhs, double rhs) {
    return static_cast<double>(lhs) < rhs;
}

template <size_t N, size_t K, typename Tag>
inline bool operator<(const FixedPoint<N, K, Tag>& lhs, int rhs) {
    return static_cast<double>(lhs) < rhs;
}

template <size_t N, size_t K, typename Tag>
inline bool operator<(double lhs, const FixedPoint<N, K, Tag>& rhs) {
    return lhs < static_cast<double>(rhs);
}

template <size_t N, size_t K, typename Tag>
constexpr bool operator>(const FixedPoint<N, K, Tag>& lhs, double rhs) {
    return static_cast<double>(lhs) > rhs;
}

template <size_t N, size_t K, typename Tag>
constexpr bool operator>(const FixedPoint<N, K, Tag>& lhs, int rhs) {
    return static_cast<double>(lhs) > rhs;
}

template <size_t N, size_t K, typename Tag>
bool operator>(const FixedPoint<N, K>& lhs, double rhs) {
    return static_cast<double>(lhs) > rhs;
}

template <size_t N, size_t K, typename Tag>
bool operator>(double lhs, const FixedPoint<N, K>& rhs) {
    return lhs > static_cast<double>(rhs);
}

template <size_t N, size_t K, typename Tag>
inline bool operator>(double lhs, const FixedPoint<N, K, Tag>& rhs) {
    return lhs > static_cast<double>(rhs);
}