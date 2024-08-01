#pragma once

#include "bx/engine/core/byte_types.hpp"

#include <cmath>

#define SAFE_DIV_EPSILON 0.00001

namespace Math
{
	static constexpr f64 SAVE_DIV_EPSILON = 0.00001;

	static constexpr f64 E = 2.71828182845904523536;		// e
	static constexpr f64 LOG2E = 1.44269504088896340736;	// log2(e)
	static constexpr f64 LOG10E = 0.434294481903251827651;	// log10(e)
	static constexpr f64 LN2 = 0.693147180559945309417;		// ln(2)
	static constexpr f64 LN10 = 2.30258509299404568402;		// ln(10)

	static constexpr f64 PI = 3.14159265358979323846;		// pi
	static constexpr f64 PI_2 = 1.57079632679489661923;		// pi/2
	static constexpr f64 PI_4 = 0.785398163397448309616;	// pi/4
	static constexpr f64 INV_PI = 0.318309886183790671538;	// 1/pi
	static constexpr f64 SQRT2 = 1.41421356237309504880;	// sqrt(2)
	static constexpr f64 SQRT1_2 = 0.707106781186547524401;	// 1/sqrt(2)


	template <typename T>
	static T FMod(const T& a, const T& b)
	{
		return std::fmod(a, b);
	}

	template <typename T>
	static T Max(const T& a, const T& b)
	{
		return (a > b) ? a : b;
	}

	template <typename T>
	static T Min(const T& a, const T& b)
	{
		return (a < b) ? a : b;
	}

	template <typename T>
	static T Clamp(const T& x, const T& min, const T& max)
	{
		return Max(Min(x, max), min);
	}

	template <typename T>
	static T Lerp(const T& a, const T& b, f32 t)
	{
		return a + (b - a) * t;
	}

	template <typename T>
	static T Degrees(const T& x)
	{
		return x * static_cast<T>(180.0 / 3.14159265359);
	}

	template <typename T>
	static T Radians(const T& x)
	{
		return x * static_cast<T>(3.14159265359 / 180.0);
	}

	template <typename T>
	static T Pow2(const T& x)
	{
		return x * x;
	}

	template <typename T>
	static T Pow3(const T& x)
	{
		return Pow2(x) * x;
	}

	template <typename T>
	static T Pow4(const T& x)
	{
		T x2 = Pow2(x);
		return Pow2(x2);
	}

	template <typename T>
	static T Pow5(const T& x)
	{
		return Pow4(x) * x;
	}

	static u32 DivCeil(u32 x, u32 y)
	{
		return (x + y - 1) / y;
	}
}

struct Vec2
{
	Vec2() : data{ 0, 0 } {}
	Vec2(f32 x, f32 y)
		: data{ x, y }
	{}

	union
	{
		f32 data[2];
		struct { f32 x, y; };
	};

	f32 At(i32 i) const;
	inline f32& operator[](i32 i) { return data[i]; }
	inline const f32& operator[](i32 i) const { return data[i]; }

	f32 SqrMagnitude() const;
	f32 Magnitude() const;
	Vec2 Normalized() const;
	Vec2 Abs() const;

	void Set(f32 x, f32 y);

	Vec2 AddScalar(f32 rhs) const;
	Vec2 Add(const Vec2& rhs) const;
	inline Vec2 operator+(const f32& rhs) const { return AddScalar(rhs); }
	inline Vec2& operator+=(const f32& rhs) { *this = *this + rhs; return *this; }
	inline Vec2 operator+(const Vec2& rhs) const { return Add(rhs); }
	inline Vec2& operator+=(const Vec2& rhs) { *this = *this + rhs; return *this; }

	Vec2 Negate() const;
	inline Vec2 operator-() const { return Negate(); }

	Vec2 SubScalar(f32 rhs) const;
	Vec2 Sub(const Vec2& rhs) const;
	inline Vec2 operator-(const f32& rhs) const { return SubScalar(rhs); }
	inline Vec2& operator-=(const f32& rhs) { *this = *this - rhs; return *this; }
	inline Vec2 operator-(const Vec2& rhs) const { return Sub(rhs); }
	inline Vec2& operator-=(const Vec2& rhs) { *this = *this - rhs; return *this; }

	Vec2 MulF32(f32 rhs) const;
	Vec2 Mul(const Vec2& rhs) const;
	inline Vec2 operator*(const f32& rhs) const { return MulF32(rhs); }
	inline Vec2& operator*=(const f32& rhs) { *this = *this * rhs; return *this; }
	inline Vec2 operator*(const Vec2& rhs) const { return Mul(rhs); }
	inline Vec2& operator*=(const Vec2& rhs) { *this = *this * rhs; return *this; }

	Vec2 DivF32(f32 rhs) const;
	Vec2 Div(const Vec2& rhs) const;
	inline Vec2 operator/(const f32& rhs) const { return DivF32(rhs); }
	inline Vec2& operator/=(const f32& rhs) { *this = *this / rhs; return *this; }
	inline Vec2 operator/(const Vec2& rhs) const { return Div(rhs); }
	inline Vec2& operator/=(const Vec2& rhs) { *this = *this / rhs; return *this; }

	static Vec2 Splat(f32 x) { return Vec2(x, x); }
	static Vec2 Zero() { return Vec2(0, 0); }
	static Vec2 One() { return Vec2(1, 1); }
	static Vec2 Right() { return Vec2(1, 0); }
	static Vec2 Up() { return Vec2(0, 1); }

	static f32 Dot(const Vec2& a, const Vec2& b);

	static Vec2 Lerp(const Vec2& a, const Vec2& b, f32 t);

	static void Normalize(Vec2& v);

	static Vec2 FromValuePtr(f32* v);
};

struct Vec3
{
	Vec3() : data{ 0, 0, 0 } {}
	Vec3(f32 x, f32 y, f32 z)
		: data{ x, y, z }
	{}

	union
	{
		f32 data[3];
		struct { f32 x, y, z; };
	};

	f32 At(i32 i) const;
	inline f32& operator[](i32 i) { return data[i]; }
	inline const f32& operator[](i32 i) const { return data[i]; }

	f32 SqrMagnitude() const;
	f32 Magnitude() const;
	Vec3 Normalized() const;
	Vec3 Abs() const;

	void Set(f32 x, f32 y, f32 z);

	Vec3 AddScalar(f32 rhs) const;
	Vec3 Add(const Vec3& rhs) const;
	inline Vec3 operator+(const f32& rhs) const { return AddScalar(rhs); }
	inline Vec3& operator+=(const f32& rhs) { *this = *this + rhs; return *this; }
	inline Vec3 operator+(const Vec3& rhs) const { return Add(rhs); }
	inline Vec3& operator+=(const Vec3& rhs) { *this = *this + rhs; return *this; }

	Vec3 Negate() const;
	inline Vec3 operator-() const { return Negate(); }

	Vec3 SubScalar(f32 rhs) const;
	Vec3 Sub(const Vec3& rhs) const;
	inline Vec3 operator-(const f32& rhs) const { return SubScalar(rhs); }
	inline Vec3& operator-=(const f32& rhs) { *this = *this - rhs; return *this; }
	inline Vec3 operator-(const Vec3& rhs) const { return Sub(rhs); }
	inline Vec3& operator-=(const Vec3& rhs) { *this = *this - rhs; return *this; }

	Vec3 MulF32(f32 rhs) const;
	Vec3 Mul(const Vec3& rhs) const;
	inline Vec3 operator*(const f32& rhs) const { return MulF32(rhs); }
	inline Vec3& operator*=(const f32& rhs) { *this = *this * rhs; return *this; }
	inline Vec3 operator*(const Vec3& rhs) const { return Mul(rhs); }
	inline Vec3& operator*=(const Vec3& rhs) { *this = *this * rhs; return *this; }

	Vec3 DivF32(f32 rhs) const;
	Vec3 Div(const Vec3& rhs) const;
	inline Vec3 operator/(const f32& rhs) const { return DivF32(rhs); }
	inline Vec3& operator/=(const f32& rhs) { *this = *this / rhs; return *this; }
	inline Vec3 operator/(const Vec3& rhs) const { return Div(rhs); }
	inline Vec3& operator/=(const Vec3& rhs) { *this = *this / rhs; return *this; }

	static Vec3 Splat(f32 x) { return Vec3(x, x, x); }
	static Vec3 Zero() { return Vec3(0, 0, 0); }
	static Vec3 One() { return Vec3(1, 1, 1); }
	static Vec3 Right() { return Vec3(1, 0, 0); }
	static Vec3 Up() { return Vec3(0, 1, 0); }
	static Vec3 Forward() { return Vec3(0, 0, 1); }

	static f32 Dot(const Vec3& a, const Vec3& b);

	static Vec3 Lerp(const Vec3& a, const Vec3& b, f32 t);

	static void Normalize(Vec3& v);

	static Vec3 Cross(const Vec3& a, const Vec3& b);

	static Vec3 FromValuePtr(f32* v);
};

struct Vec4
{
	Vec4() : data{ 0, 0, 0, 0 } {}
	Vec4(f32 x, f32 y, f32 z, f32 w)
		: data{ x, y, z, w }
	{}

	union
	{
		f32 data[4];
		struct { f32 x, y, z, w; };
	};

	f32 At(i32 i) const;
	inline f32& operator[](i32 i) { return data[i]; }
	inline const f32& operator[](i32 i) const { return data[i]; }

	f32 SqrMagnitude() const;
	f32 Magnitude() const;
	Vec4 Normalized() const;
	Vec4 Abs() const;

	void Set(f32 x, f32 y, f32 z, f32 w);

	Vec4 AddScalar(f32 rhs) const;
	Vec4 Add(const Vec4& rhs) const;
	inline Vec4 operator+(const f32& rhs) const { return AddScalar(rhs); }
	inline Vec4& operator+=(const f32& rhs) { *this = *this + rhs; return *this; }
	inline Vec4 operator+(const Vec4& rhs) const { return Add(rhs); }
	inline Vec4& operator+=(const Vec4& rhs) { *this = *this + rhs; return *this; }

	Vec4 Negate() const;
	inline Vec4 operator-() const { return Negate(); }

	Vec4 SubScalar(f32 rhs) const;
	Vec4 Sub(const Vec4& rhs) const;
	inline Vec4 operator-(const f32& rhs) const { return SubScalar(rhs); }
	inline Vec4& operator-=(const f32& rhs) { *this = *this - rhs; return *this; }
	inline Vec4 operator-(const Vec4& rhs) const { return Sub(rhs); }
	inline Vec4& operator-=(const Vec4& rhs) { *this = *this - rhs; return *this; }

	Vec4 MulF32(f32 rhs) const;
	Vec4 Mul(const Vec4& rhs) const;
	inline Vec4 operator*(const f32& rhs) const { return MulF32(rhs); }
	inline Vec4& operator*=(const f32& rhs) { *this = *this * rhs; return *this; }
	inline Vec4 operator*(const Vec4& rhs) const { return Mul(rhs); }
	inline Vec4& operator*=(const Vec4& rhs) { *this = *this * rhs; return *this; }

	Vec4 DivF32(f32 rhs) const;
	Vec4 Div(const Vec4& rhs) const;
	inline Vec4 operator/(const f32& rhs) const { return DivF32(rhs); }
	inline Vec4& operator/=(const f32& rhs) { *this = *this / rhs; return *this; }
	inline Vec4 operator/(const Vec4& rhs) const { return Div(rhs); }
	inline Vec4& operator/=(const Vec4& rhs) { *this = *this / rhs; return *this; }

	static Vec4 Splat(f32 x) { return Vec4(x, x, x, x); }
	static Vec4 Zero() { return Vec4(0, 0, 0, 0); }
	static Vec4 One() { return Vec4(1, 1, 1, 1); }

	static f32 Dot(const Vec4& a, const Vec4& b);

	static Vec4 Lerp(const Vec4& a, const Vec4& b, f32 t);

	static void Normalize(Vec4& v);

	static Vec4 FromValuePtr(f32* v);
};

struct Color
{
	Color() : data{ 0, 0, 0, 0 } {}
	Color(f32 r, f32 g, f32 b, f32 a)
		: data{ r, g, b, a }
	{}

	union
	{
		f32 data[4];
		struct { f32 r, g, b, a; };
	};

	static Color Black() { return Color(0, 0, 0, 1); }
	static Color Blue() { return Color(0, 0, 1, 1); }
	static Color Cyan() { return Color(0, 1, 1, 1); }
	static Color Gray() { return Color(0.5, 0.5, 0.5, 1); }
	static Color Green() { return Color(0, 1, 0, 1); }
	static Color Magenta() { return Color(1, 0, 1, 1); }
	static Color Red() { return Color(1, 0, 0, 1); }
	static Color Transparent() { return Color(0, 0, 0, 0); }
	static Color White() { return Color(1, 1, 1, 1); }
	static Color Yellow() { return Color(1, 1, 0, 1); }

	f32 At(i32 i) const;
	inline f32& operator[](i32 i) { return data[i]; }
	inline const f32& operator[](i32 i) const { return data[i]; }

	static Color FromValuePtr(f32* v);
};

struct Vec4i
{
	Vec4i() : data{ 0, 0, 0, 0 } {}
	Vec4i(i32 x, i32 y, i32 z, i32 w)
		: data{ x, y, z, w }
	{}

	union
	{
		i32 data[4];
		struct { i32 x, y, z, w; };
	};

	//i32 At(i32 i);
	inline i32& operator[](i32 i) { return data[i]; }
	inline const i32& operator[](i32 i) const { return data[i]; }

	static Vec4i FromValuePtr(i32* v);
};

struct Mat4;
struct Quat
{
	Quat() : data{ 0, 0, 0, 0 } {}
	Quat(f32 x, f32 y, f32 z, f32 w)
		: data{ x, y, z, w }
	{}

	union
	{
		f32 data[4];
		struct { f32 x, y, z, w; };
	};

	f32 At(i32 i) const;
	inline f32& operator[](i32 i) { return data[i]; }
	inline const f32& operator[](i32 i) const { return data[i]; }

	Quat Normalized() const;
	f32 SqrMagnitude() const;
	f32 Magnitude() const;
	Quat Inverse() const;

	Quat PlusQuat(const Quat& rhs) const;
	Quat AddScalar(f32 rhs) const;
	inline Quat operator+(const Quat& rhs) const { return PlusQuat(rhs); }
	inline Quat operator+(const f32& rhs) const { return AddScalar(rhs); }

	Quat Negate() const;
	inline Quat operator-() const { return Negate(); }

	Quat MulQuat(const Quat& rhs) const;
	Vec3 MulVec3(const Vec3& rhs) const;
	Quat MulF32(f32 rhs) const;
	inline Quat operator*(const Quat& rhs) const { return MulQuat(rhs); }
	inline Quat& operator*=(const Quat& rhs) { *this = *this * rhs; return *this; }
	inline Vec3 operator*(const Vec3& rhs) const { return MulVec3(rhs); }
	inline Quat operator*(const f32& rhs) const { return MulF32(rhs); }

	Quat DivF32(f32 rhs) const;
	inline Quat operator/(const f32& rhs) const { return DivF32(rhs); }

	static Quat Splat(f32 x) { return Quat(x, x, x, x); }
	static Quat Zero() { return Quat(0, 0, 0, 0); }
	static Quat One() { return Quat(1, 1, 1, 1); }
	static Quat Identity() { return Quat(0, 0, 0, 1); }

	Vec3 EulerAngles() const;

	static Quat Euler(f32 x, f32 y, f32 z);
	static Quat AngleAxis(f32 angleInDegrees, const Vec3& axis);

	static f32 Dot(const Quat& a, const Quat& b);

	static Quat Slerp(const Quat& a, const Quat& b, f32 t);

	static Quat FromMat4(const Mat4& matrix);
	static Quat FromValuePtr(f32* v);
};

struct Mat4
{
	Mat4() : data{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } {}
	Mat4(const Vec4& x, const Vec4& y, const Vec4& z, const Vec4& w)
		: basis{ x, y, z, w }
	{}

	union
	{
		f32 data[16];
		Vec4 basis[4];
	};

	f32 At(i32 i) const;
	f32 At(i32 i, i32 j) const;
	inline f32& operator[](i32 i) { return data[i]; }
	inline const f32& operator[](i32 i) const { return data[i]; }
	f32& operator()(u32 i, u32 j) { return basis[i][j]; }
	const f32& operator()(u32 i, u32 j) const { return basis[i][j]; }

	Mat4 Mul(const Mat4& rhs) const;
	Vec4 MulVec4(const Vec4& rhs) const;
	inline Mat4 operator*(const Mat4& rhs) const { return Mul(rhs); }
	inline Vec4 operator*(const Vec4& rhs) const { return MulVec4(rhs); }

	Mat4 Transpose() const;
	Mat4 Inverse() const;

	static Mat4 Identity();
	static Mat4 Zero();

	static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up);

	static Mat4 Ortho(f32 left, f32 right, f32 bottom, f32 top, f32 zNear, f32 zFar);
	static Mat4 Perspective(f32 fov, f32 aspect, f32 zNear, f32 zFar);
	static Mat4 Translation(const Vec3& translation);
	static Mat4 Rotation(f32 angle, const Vec3& axis);
	static Mat4 Rotation(const Quat& rotation);
	static Mat4 Scale(const Vec3& scale);
	static Mat4 TRS(const Vec3& translation, const Quat& rotation, const Vec3& scale);

	static void Decompose(const Mat4& matrix, Vec3& translation, Quat& rotation, Vec3& scale);

	static Mat4 FromValuePtr(f32* v);
};

struct Box3
{
	Box3() : min(0, 0, 0), max(0, 0, 0) {}
	Box3(Vec3 min, Vec3 max)
		: min(min)
		, max(max)
	{}

	Vec3 min;
	Vec3 max;

	inline bool Overlaps(const Box3& other) const
	{
		return min.x <= other.max.x && max.x >= other.min.x
			&& min.y <= other.max.y && max.y >= other.min.y
			&& min.z <= other.max.z && max.z >= other.min.z;
	}
};