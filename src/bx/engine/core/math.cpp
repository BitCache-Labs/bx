#include "bx/engine/core/math.hpp"

#define GLM_LANG_STL11_FORCED
#define GLM_ENABLE_EXPERIMENTAL
//#define GLM_FORCE_QUAT_DATA_XYZW

#include <glm/glm.hpp>

#include <glm/gtx/hash.hpp>
#include <glm/gtx/string_cast.hpp>

#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

// FIXME: There is a bug with glm when GLM_FORCE_QUAT_DATA_XYZW is defined
#define GLM_PATCH_QUAT_DATA_XYZW

#ifdef GLM_PATCH_QUAT_DATA_XYZW
static Quat QuatFromGLM(glm::quat q)
{
	return Quat(q.x, q.y, q.z, q.w);
}

static glm::quat QuatToGLM(const Quat& q)
{
	return glm::quat(q.w, q.x, q.y, q.z);
}
#else
static Quat QuatFromGLM(glm::quat q)
{
	return Quat::FromValuePtr(glm::value_ptr(q));
}

static glm::quat QuatToGLM(const Quat& q)
{
	return glm::make_quat(q.data);
}
#endif

#define USE_GLM_IMPL

#ifdef USE_GLM_IMPL

f32 Vec2::At(i32 i) const
{
	return data[i];
}

f32 Vec2::SqrMagnitude() const
{
	glm::vec2 v = glm::make_vec2(data);
	return glm::dot(v, v);
}

f32 Vec2::Magnitude() const
{
	glm::vec2 v = glm::make_vec2(data);
	return glm::length(v);
}

Vec2 Vec2::Normalized() const
{
	glm::vec2 v = glm::normalize(glm::make_vec2(data));
	return Vec2::FromValuePtr(glm::value_ptr(v));
}

Vec2 Vec2::Abs() const
{
	return Vec2(fabsf(x), fabsf(y));
}

Vec2 Vec2::Yx() const
{
	return Vec2(y, x);
}

void Vec2::Set(f32 x, f32 y)
{
	data[0] = x;
	data[1] = y;
}

Vec2 Vec2::AddScalar(f32 rhs) const
{
	glm::vec2 v = glm::make_vec2(data) + rhs;
	return Vec2::FromValuePtr(glm::value_ptr(v));
}

Vec2 Vec2::Add(const Vec2& rhs) const
{
	glm::vec2 v = glm::make_vec2(data) + glm::make_vec2(rhs.data);
	return Vec2::FromValuePtr(glm::value_ptr(v));
}

Vec2 Vec2::Negate() const
{
	return Vec2(-x, -y);
}

Vec2 Vec2::SubScalar(f32 rhs) const
{
	glm::vec2 v = glm::make_vec2(data) - rhs;
	return Vec2::FromValuePtr(glm::value_ptr(v));
}

Vec2 Vec2::Sub(const Vec2& rhs) const
{
	glm::vec2 v = glm::make_vec2(data) - glm::make_vec2(rhs.data);
	return Vec2::FromValuePtr(glm::value_ptr(v));
}

Vec2 Vec2::MulF32(f32 rhs) const
{
	glm::vec2 v = glm::make_vec2(data) * rhs;
	return Vec2::FromValuePtr(glm::value_ptr(v));
}

Vec2 Vec2::Mul(const Vec2& rhs) const
{
	glm::vec2 v = glm::make_vec2(data) * glm::make_vec2(rhs.data);
	return Vec2::FromValuePtr(glm::value_ptr(v));
}

Vec2 Vec2::DivF32(f32 rhs) const
{
	glm::vec2 v = glm::make_vec2(data) / rhs;
	return Vec2::FromValuePtr(glm::value_ptr(v));
}

Vec2 Vec2::Div(const Vec2& rhs) const
{
	glm::vec2 v = glm::make_vec2(data) / glm::make_vec2(rhs.data);
	return Vec2::FromValuePtr(glm::value_ptr(v));
}

f32 Vec2::Dot(const Vec2& a, const Vec2& b)
{
	return glm::dot(glm::make_vec2(a.data), glm::make_vec2(b.data));
}

Vec2 Vec2::Lerp(const Vec2& a, const Vec2& b, f32 t)
{
	glm::vec2 v = glm::mix(glm::make_vec2(a.data), glm::make_vec2(b.data), t);
	return Vec2::FromValuePtr(glm::value_ptr(v));
}

void Vec2::Normalize(Vec2& v)
{
	v = v.Normalized();
}

Vec2 Vec2::FromValuePtr(f32* vptr)
{
	Vec2 v;
	memcpy(v.data, vptr, sizeof(Vec2));
	return v;
}

f32 Vec3::At(i32 i) const
{
	return data[i];
}

f32 Vec3::SqrMagnitude() const
{
	glm::vec3 v = glm::make_vec3(data);
	return glm::dot(v, v);
}

f32 Vec3::Magnitude() const
{
	glm::vec3 v = glm::make_vec3(data);
	return glm::length(v);
}

Vec3 Vec3::Normalized() const
{
	glm::vec3 v = glm::normalize(glm::make_vec3(data));
	return Vec3::FromValuePtr(glm::value_ptr(v));
}

Vec3 Vec3::Abs() const
{
	return Vec3(fabsf(x), fabsf(y), fabsf(z));
}

Vec2 Vec3::Xy() const
{
	return Vec2(x, y);
}

Vec2 Vec3::Yx() const
{
	return Vec2(y, x);
}

void Vec3::Set(f32 x, f32 y, f32 z)
{
	data[0] = x;
	data[1] = y;
	data[2] = z;
}

Vec3 Vec3::AddScalar(f32 rhs) const
{
	glm::vec3 v = glm::make_vec3(data) + rhs;
	return Vec3::FromValuePtr(glm::value_ptr(v));
}

Vec3 Vec3::Add(const Vec3& rhs) const
{
	glm::vec3 v = glm::make_vec3(data) + glm::make_vec3(rhs.data);
	return Vec3::FromValuePtr(glm::value_ptr(v));
}

Vec3 Vec3::Negate() const
{
	return Vec3(-x, -y, -z);
}

Vec3 Vec3::SubScalar(f32 rhs) const
{
	glm::vec3 v = glm::make_vec3(data) - rhs;
	return Vec3::FromValuePtr(glm::value_ptr(v));
}

Vec3 Vec3::Sub(const Vec3& rhs) const
{
	glm::vec3 v = glm::make_vec3(data) - glm::make_vec3(rhs.data);
	return Vec3::FromValuePtr(glm::value_ptr(v));
}

Vec3 Vec3::MulF32(f32 rhs) const
{
	glm::vec3 v = glm::make_vec3(data) * rhs;
	return Vec3::FromValuePtr(glm::value_ptr(v));
}

Vec3 Vec3::Mul(const Vec3& rhs) const
{
	glm::vec3 v = glm::make_vec3(data) * glm::make_vec3(rhs.data);
	return Vec3::FromValuePtr(glm::value_ptr(v));
}

Vec3 Vec3::DivF32(f32 rhs) const
{
	glm::vec3 v = glm::make_vec3(data) / rhs;
	return Vec3::FromValuePtr(glm::value_ptr(v));
}

Vec3 Vec3::Div(const Vec3& rhs) const
{
	glm::vec3 v = glm::make_vec3(data) / glm::make_vec3(rhs.data);
	return Vec3::FromValuePtr(glm::value_ptr(v));
}

Vec3 Vec3::Clamp(const Vec3& x, const Vec3& lo, const Vec3& hi)
{
	return Vec3(
		Math::Clamp(x.x, lo.x, hi.x),
		Math::Clamp(x.y, lo.y, hi.y),
		Math::Clamp(x.z, lo.z, hi.z)
	);
}

f32 Vec3::Dot(const Vec3& a, const Vec3& b)
{
	return glm::dot(glm::make_vec3(a.data), glm::make_vec3(b.data));
}

Vec3 Vec3::Lerp(const Vec3& a, const Vec3& b, f32 t)
{
	glm::vec3 v = glm::mix(glm::make_vec3(a.data), glm::make_vec3(b.data), t);
	return Vec3::FromValuePtr(glm::value_ptr(v));
}

void Vec3::Normalize(Vec3& v)
{
	v = v.Normalized();
}

Vec3 Vec3::Cross(const Vec3& a, const Vec3& b)
{
	glm::vec3 v = glm::cross(glm::make_vec3(a.data), glm::make_vec3(b.data));
	return Vec3::FromValuePtr(glm::value_ptr(v));
}

Vec3 Vec3::FromValuePtr(f32* vptr)
{
	Vec3 v;
	memcpy(v.data, vptr, sizeof(Vec3));
	return v;
}

Vec3u Vec3u::FromValuePtr(u32* vptr)
{
	Vec3u v;
	memcpy(v.data, vptr, sizeof(Vec3u));
	return v;
}

f32 Vec4::At(i32 i) const
{
	return data[i];
}

f32 Vec4::SqrMagnitude() const
{
	glm::vec4 v = glm::make_vec4(data);
	return glm::dot(v, v);
}

f32 Vec4::Magnitude() const
{
	glm::vec4 v = glm::make_vec4(data);
	return glm::length(v);
}

Vec4 Vec4::Normalized() const
{
	glm::vec4 v = glm::normalize(glm::make_vec4(data));
	return Vec4::FromValuePtr(glm::value_ptr(v));
}

Vec4 Vec4::Abs() const
{
	return Vec4(fabsf(x), fabsf(y), fabsf(z), fabsf(w));
}

Vec2 Vec4::Xy() const
{
	return Vec2(x, y);
}

Vec3 Vec4::Xyz() const
{
	return Vec3(x, y, z);
}

void Vec4::Set(f32 x, f32 y, f32 z, f32 w)
{
	data[0] = x;
	data[1] = y;
	data[2] = z;
	data[3] = w;
}

Vec4 Vec4::AddScalar(f32 rhs) const
{
	glm::vec4 v = glm::make_vec4(data) + rhs;
	return Vec4::FromValuePtr(glm::value_ptr(v));
}

Vec4 Vec4::Add(const Vec4& rhs) const
{
	glm::vec4 v = glm::make_vec4(data) + glm::make_vec4(rhs.data);
	return Vec4::FromValuePtr(glm::value_ptr(v));
}

Vec4 Vec4::Negate() const
{
	return Vec4(-x, -y, -z, -w);
}

Vec4 Vec4::SubScalar(f32 rhs) const
{
	glm::vec4 v = glm::make_vec4(data) - rhs;
	return Vec4::FromValuePtr(glm::value_ptr(v));
}

Vec4 Vec4::Sub(const Vec4& rhs) const
{
	glm::vec4 v = glm::make_vec4(data) - glm::make_vec4(rhs.data);
	return Vec4::FromValuePtr(glm::value_ptr(v));
}

Vec4 Vec4::MulF32(f32 rhs) const
{
	glm::vec4 v = glm::make_vec4(data) * rhs;
	return Vec4::FromValuePtr(glm::value_ptr(v));
}

Vec4 Vec4::Mul(const Vec4& rhs) const
{
	glm::vec4 v = glm::make_vec4(data) * glm::make_vec4(rhs.data);
	return Vec4::FromValuePtr(glm::value_ptr(v));
}

Vec4 Vec4::DivF32(f32 rhs) const
{
	glm::vec4 v = glm::make_vec4(data) / rhs;
	return Vec4::FromValuePtr(glm::value_ptr(v));
}

Vec4 Vec4::Div(const Vec4& rhs) const
{
	glm::vec4 v = glm::make_vec4(data) / glm::make_vec4(rhs.data);
	return Vec4::FromValuePtr(glm::value_ptr(v));
}

f32 Vec4::Dot(const Vec4& a, const Vec4& b)
{
	return glm::dot(glm::make_vec4(a.data), glm::make_vec4(b.data));
}

Vec4 Vec4::Lerp(const Vec4& a, const Vec4& b, f32 t)
{
	glm::vec4 v = glm::mix(glm::make_vec4(a.data), glm::make_vec4(b.data), t);
	return Vec4::FromValuePtr(glm::value_ptr(v));
}

void Vec4::Normalize(Vec4& v)
{
	v = v.Normalized();
}

Vec4 Vec4::FromValuePtr(f32* vptr)
{
	Vec4 v;
	memcpy(v.data, vptr, sizeof(Vec4));
	return v;
}

Vec3 Color::Xyz() const
{
	return Vec3(r, g, b);
}

f32 Color::At(i32 i) const
{
	return data[i];
}

Color Color::FromValuePtr(f32* vptr)
{
	Color v;
	memcpy(v.data, vptr, sizeof(Color));
	return v;
}

Vec4i Vec4i::FromValuePtr(i32* vptr)
{
	Vec4i v;
	memcpy(v.data, vptr, sizeof(Vec4i));
	return v;
}

f32 Quat::At(i32 i) const
{
	return data[i];
}

Quat Quat::Normalized() const
{
	glm::quat q = glm::normalize(QuatToGLM(*this));
	return QuatFromGLM(q);
}

f32 Quat::SqrMagnitude() const
{
	return x * x + y * y + z * z + w * w;
}

f32 Quat::Magnitude() const
{
	return glm::length(QuatToGLM(*this));
}

Quat Quat::Inverse() const
{
	glm::quat q = glm::inverse(QuatToGLM(*this));
	return QuatFromGLM(q);
}

Quat Quat::PlusQuat(const Quat& rhs) const
{
	glm::quat q = QuatToGLM(*this) + QuatToGLM(rhs);
	return QuatFromGLM(q);
}

Quat Quat::AddScalar(f32 rhs) const
{
	return Quat(x + rhs, y + rhs, z + rhs, w + rhs);
}

Quat Quat::Negate() const
{
	return Quat(-x, -y, -z, -w);
}

Quat Quat::MulQuat(const Quat& rhs) const
{
	glm::quat q = QuatToGLM(*this) * QuatToGLM(rhs);
	return QuatFromGLM(q);
}

Vec3 Quat::MulVec3(const Vec3& rhs) const
{
	glm::vec3 v = QuatToGLM(*this) * glm::make_vec3(rhs.data);
	return Vec3::FromValuePtr(glm::value_ptr(v));
}

Quat Quat::MulF32(f32 rhs) const
{
	return Quat(x * rhs, y * rhs, z * rhs, w * rhs);
}

Quat Quat::DivF32(f32 rhs) const
{
	f32 invRhs = 1.0 / rhs;
	return Quat(x * invRhs, y * invRhs, z * invRhs, w * invRhs);
}

Vec3 Quat::EulerAngles() const
{
	glm::quat q = QuatToGLM(*this);
	glm::vec3 e = glm::degrees(glm::eulerAngles(q));
	return Vec3::FromValuePtr(glm::value_ptr(e));
}

Quat Quat::Euler(f32 x, f32 y, f32 z)
{
	glm::quat q = glm::quat(glm::radians(glm::vec3(x, y, z)));
	return QuatFromGLM(q);
}

Quat Quat::AngleAxis(f32 angleInDegrees, const Vec3& axis)
{
	glm::quat q = glm::angleAxis(glm::radians(angleInDegrees), glm::vec3(axis.x, axis.y, axis.z));
	return QuatFromGLM(q);
}

f32 Quat::Dot(const Quat& a, const Quat& b)
{
	return glm::dot(QuatToGLM(a), QuatToGLM(b));
}

Quat Quat::Slerp(const Quat& a, const Quat& b, f32 t)
{
	glm::quat qa = QuatToGLM(a);
	glm::quat qb = QuatToGLM(b);
	glm::quat qs = glm::slerp(qa, qb, t);
	return QuatFromGLM(qs);
}

Quat Quat::FromMat4(const Mat4& _matrix)
{
	glm::mat4 matrix = glm::make_mat4(_matrix.data);
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(matrix, scale, rotation, translation, skew, perspective);
	return QuatFromGLM(rotation);
}

Quat Quat::FromValuePtr(f32* vptr)
{
	Quat q;
	memcpy(q.data, vptr, sizeof(Quat));
	return q;
}

f32 Mat4::At(i32 i) const
{
	return data[i];
}

f32 Mat4::At(i32 i, i32 j) const
{
	return basis[i][j];
}

Mat4 Mat4::Mul(const Mat4& rhs) const
{
	glm::mat4 m = glm::make_mat4(data) * glm::make_mat4(rhs.data);
	return Mat4::FromValuePtr(glm::value_ptr(m));
}

Vec4 Mat4::MulVec4(const Vec4& rhs) const
{
	glm::vec4 v = glm::make_mat4(data) * glm::make_vec4(rhs.data);
	return Vec4::FromValuePtr(glm::value_ptr(v));
}

Mat4 Mat4::Transpose() const
{
	glm::mat4 m = glm::make_mat4(data);
	glm::mat4 im = glm::transpose(m);
	return Mat4::FromValuePtr(glm::value_ptr(im));
}

Mat4 Mat4::Inverse() const
{
	glm::mat4 m = glm::make_mat4(data);
	glm::mat4 im = glm::inverse(m);
	return Mat4::FromValuePtr(glm::value_ptr(im));
}

Mat4 Mat4::Identity()
{
	static Mat4 s_identity = Mat4(
		Vec4(1, 0, 0, 0),
		Vec4(0, 1, 0, 0),
		Vec4(0, 0, 1, 0),
		Vec4(0, 0, 0, 1)
	);
	return s_identity;
}

Mat4 Mat4::Zero()
{
	return Mat4(
		Vec4(0, 0, 0, 0),
		Vec4(0, 0, 0, 0),
		Vec4(0, 0, 0, 0),
		Vec4(0, 0, 0, 0)
	);
}

Mat4 Mat4::LookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
{
	glm::vec3 e = glm::make_vec3(eye.data);
	glm::vec3 c = glm::make_vec3(center.data);
	glm::vec3 u = glm::make_vec3(up.data);
	glm::mat4 m = glm::lookAt(e, c, u);
	return Mat4::FromValuePtr(glm::value_ptr(m));
}

Mat4 Mat4::Ortho(f32 left, f32 right, f32 bottom, f32 top, f32 zNear, f32 zFar)
{
	glm::mat4 m = glm::ortho(left, right, bottom, top, zNear, zFar);
	return Mat4::FromValuePtr(glm::value_ptr(m));
}

Mat4 Mat4::Perspective(f32 fov, f32 aspect, f32 zNear, f32 zFar)
{
	glm::mat4 m = glm::perspective(glm::radians(fov), aspect, zNear, zFar);
	return Mat4::FromValuePtr(glm::value_ptr(m));
}

Mat4 Mat4::Translation(const Vec3& translation)
{
	glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::make_vec3(translation.data));
	return Mat4::FromValuePtr(glm::value_ptr(m));
}

Mat4 Mat4::Rotation(f32 angle, const Vec3& axis)
{
	glm::mat4 m = glm::mat4(glm::angleAxis(glm::radians(angle), glm::make_vec3(axis.data)));
	return Mat4::FromValuePtr(glm::value_ptr(m));
}

Mat4 Mat4::Rotation(const Quat& rotation)
{
	glm::mat4 m = glm::mat4(QuatToGLM(rotation));
	return Mat4::FromValuePtr(glm::value_ptr(m));
}

Mat4 Mat4::Scale(const Vec3& scale)
{
	glm::mat4 m = glm::scale(glm::mat4(1.0f), glm::make_vec3(scale.data));
	return Mat4::FromValuePtr(glm::value_ptr(m));
}

Mat4 Mat4::TRS(const Vec3& translation, const Quat& rotation, const Vec3& scale)
{
	return Mat4::Translation(translation) * Mat4::Rotation(rotation) * Mat4::Scale(scale);
}

void Mat4::Decompose(const Mat4& matrix, Vec3& pos, Quat& rot, Vec3& scl)
{
	glm::mat4 transformation = glm::make_mat4(matrix.data);
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transformation, scale, rotation, translation, skew, perspective);
	pos = Vec3::FromValuePtr(glm::value_ptr(translation));
	rot = QuatFromGLM(rotation);
	scl = Vec3::FromValuePtr(glm::value_ptr(scale));
}

Mat4 Mat4::FromValuePtr(f32* vptr)
{
	Mat4 m;
	memcpy(m.data, vptr, sizeof(Mat4));
	return m;
}

#else

f32 Vec2::At(i32 i) const
{
	return data[i];
}

f32 Vec2::SqrMagnitude() const
{
	return Vec2::Dot(*this, *this);
}

f32 Vec2::Magnitude() const
{
	return sqrtf(SqrMagnitude());
}

Vec2 Vec2::Normalized() const
{
	f32 magnitude = Magnitude();
	if (magnitude > SAFE_DIV_EPSILON)
	{
		f32 invMagnitude = 1.0 / magnitude;
		return (*this) * invMagnitude;
	}
	else
	{
		return *this;
	}
}

Vec2 Vec2::Abs() const
{
	return Vec2(fabsf(x), fabsf(y));
}

void Vec2::Set(f32 x, f32 y)
{
	data[0] = x;
	data[1] = y;
}

Vec2 Vec2::AddScalar(f32 rhs) const
{
	return Vec2(x + rhs, y + rhs);
}

Vec2 Vec2::Add(const Vec2& rhs) const
{
	return Vec2(x + rhs.x, y + rhs.y);
}

Vec2 Vec2::Negate() const
{
	return Vec2(-x, -y);
}

Vec2 Vec2::SubScalar(f32 rhs) const
{
	return Vec2(x - rhs, y - rhs);
}

Vec2 Vec2::Sub(const Vec2& rhs) const
{
	return Vec2(x - rhs.x, y - rhs.y);
}

Vec2 Vec2::MulF32(f32 rhs) const
{
	return Vec2(x * rhs, y * rhs);
}

Vec2 Vec2::Mul(const Vec2& rhs) const
{
	return Vec2(x * rhs.x, y * rhs.y);
}

Vec2 Vec2::DivF32(f32 rhs) const
{
	f32 invRhs = 1.0 / rhs;
	return Vec2(x * invRhs, y * invRhs);
}

Vec2 Vec2::Div(const Vec2& rhs) const
{
	return Vec2(x / rhs.x, y / rhs.y);
}

f32 Vec2::Dot(const Vec2& a, const Vec2& b)
{
	return a.x * b.x + a.y * b.y;
}

Vec2 Vec2::Lerp(const Vec2& a, const Vec2& b, f32 t)
{
	return Math::Lerp(a, b, t);
}

void Vec2::Normalize(Vec2& v)
{
	v = v.Normalized();
}

Vec2 Vec2::FromValuePtr(f32* vptr)
{
	Vec2 v;
	memcpy(v.data, vptr, sizeof(Vec2));
	return v;
}

f32 Vec3::At(i32 i) const
{
	return data[i];
}

f32 Vec3::SqrMagnitude() const
{
	return Vec3::Dot(*this, *this);
}

f32 Vec3::Magnitude() const
{
	return sqrtf(SqrMagnitude());
}

Vec3 Vec3::Normalized() const
{
	f32 magnitude = Magnitude();
	if (magnitude > SAFE_DIV_EPSILON)
	{
		f32 invMagnitude = 1.0 / magnitude;
		return (*this) * invMagnitude;
	}
	else
	{
		return *this;
	}
}

Vec3 Vec3::Abs() const
{
	return Vec3(fabsf(x), fabsf(y), fabsf(z));
}

void Vec3::Set(f32 x, f32 y, f32 z)
{
	data[0] = x;
	data[1] = y;
	data[2] = z;
}

Vec3 Vec3::AddScalar(f32 rhs) const
{
	return Vec3(x + rhs, y + rhs, z + rhs);
}

Vec3 Vec3::Add(const Vec3& rhs) const
{
	return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
}

Vec3 Vec3::Negate() const
{
	return Vec3(-x, -y, -z);
}

Vec3 Vec3::SubScalar(f32 rhs) const
{
	return Vec3(x - rhs, y - rhs, z - rhs);
}

Vec3 Vec3::Sub(const Vec3& rhs) const
{
	return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
}

Vec3 Vec3::MulF32(f32 rhs) const
{
	return Vec3(x * rhs, y * rhs, z * rhs);
}

Vec3 Vec3::Mul(const Vec3& rhs) const
{
	return Vec3(x * rhs.x, y * rhs.y, z * rhs.z);
}

Vec3 Vec3::DivF32(f32 rhs) const
{
	f32 invRhs = 1.0 / rhs;
	return Vec3(x * invRhs, y * invRhs, z * invRhs);
}

Vec3 Vec3::Div(const Vec3& rhs) const
{
	return Vec3(x / rhs.x, y / rhs.y, z / rhs.z);
}

f32 Vec3::Dot(const Vec3& a, const Vec3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Vec3::Lerp(const Vec3& a, const Vec3& b, f32 t)
{
	return Math::Lerp(a, b, t);
}

void Vec3::Normalize(Vec3& v)
{
	v = v.Normalized();
}

Vec3 Vec3::Cross(const Vec3& a, const Vec3& b)
{
	return Vec3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	);
}

Vec3 Vec3::FromValuePtr(f32* vptr)
{
	Vec3 v;
	memcpy(v.data, vptr, sizeof(Vec3));
	return v;
}

f32 Vec4::At(i32 i) const
{
	return data[i];
}

f32 Vec4::SqrMagnitude() const
{
	return Vec4::Dot(*this, *this);
}

f32 Vec4::Magnitude() const
{
	return sqrtf(SqrMagnitude());
}

Vec4 Vec4::Normalized() const
{
	f32 magnitude = Magnitude();
	if (magnitude > SAFE_DIV_EPSILON)
	{
		f32 invMagnitude = 1.0 / magnitude;
		return (*this) * invMagnitude;
	}
	else
	{
		return *this;
	}
}

Vec4 Vec4::Abs() const
{
	return Vec4(fabsf(x), fabsf(y), fabsf(z), fabsf(w));
}

void Vec4::Set(f32 x, f32 y, f32 z, f32 w)
{
	data[0] = x;
	data[1] = y;
	data[2] = z;
	data[3] = w;
}

Vec4 Vec4::AddScalar(f32 rhs) const
{
	return Vec4(x + rhs, y + rhs, z + rhs, w + rhs);
}

Vec4 Vec4::Add(const Vec4& rhs) const
{
	return Vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
}

Vec4 Vec4::Negate() const
{
	return Vec4(-x, -y, -z, -w);
}

Vec4 Vec4::SubScalar(f32 rhs) const
{
	return Vec4(x - rhs, y - rhs, z - rhs, w - rhs);
}

Vec4 Vec4::Sub(const Vec4& rhs) const
{
	return Vec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
}

Vec4 Vec4::MulF32(f32 rhs) const
{
	return Vec4(x * rhs, y * rhs, z * rhs, w * rhs);
}

Vec4 Vec4::Mul(const Vec4& rhs) const
{
	return Vec4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
}

Vec4 Vec4::DivF32(f32 rhs) const
{
	f32 invRhs = 1.0 / rhs;
	return Vec4(x * invRhs, y * invRhs, z * invRhs, w * invRhs);
}

Vec4 Vec4::Div(const Vec4& rhs) const
{
	return Vec4(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
}

f32 Vec4::Dot(const Vec4& a, const Vec4& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

Vec4 Vec4::Lerp(const Vec4& a, const Vec4& b, f32 t)
{
	return Math::Lerp(a, b, t);
}

void Vec4::Normalize(Vec4& v)
{
	v = v.Normalized();
}

Vec4 Vec4::FromValuePtr(f32* vptr)
{
	Vec4 v;
	memcpy(v.data, vptr, sizeof(Vec4));
	return v;
}

f32 Color::At(i32 i) const
{
	return data[i];
}

Color Color::FromValuePtr(f32* vptr)
{
	Color v;
	memcpy(v.data, vptr, sizeof(Color));
	return v;
}

Vec4i Vec4i::FromValuePtr(i32* vptr)
{
	Vec4i v;
	memcpy(v.data, vptr, sizeof(Vec4i));
	return v;
}

f32 Quat::At(i32 i) const
{
	return data[i];
}

Quat Quat::Normalized() const
{
	glm::quat q = glm::normalize(QuatToGLM(*this));
	return QuatFromGLM(q);
}

f32 Quat::SqrMagnitude() const
{
	return x * x + y * y + z * z + w * w;
}

f32 Quat::Magnitude() const
{
	return sqrt(SqrMagnitude());
}

Quat Quat::Inverse() const
{
	f32 sqrMagnitude = SqrMagnitude();
	if (sqrMagnitude > SAFE_DIV_EPSILON)
	{
		f32 invSqrMagnitude = 1.0 / sqrMagnitude;
		return Quat(
			-x * invSqrMagnitude,
			-y * invSqrMagnitude,
			-z * invSqrMagnitude,
			w * invSqrMagnitude
		);
	}
	else
	{
		return *this;
	}
}

Quat Quat::PlusQuat(const Quat& rhs) const
{
	return Quat(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
}

Quat Quat::AddScalar(f32 rhs) const
{
	return Quat(x + rhs, y + rhs, z + rhs, w + rhs);
}

Quat Quat::Negate() const
{
	return Quat(-x, -y, -z, -w);
}

Quat Quat::MulQuat(const Quat& rhs) const
{
	return Quat(
		w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
		w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
		w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w,
		w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z
	);
}

Vec3 Quat::MulVec3(const Vec3& rhs) const
{
	Vec3 qv(x, y, z);
	Vec3 t = Vec3::Cross(qv, rhs) * 2.0;
	return rhs + t * w + Vec3::Cross(qv, t);
}

Quat Quat::MulF32(f32 rhs) const
{
	return Quat(x * rhs, y * rhs, z * rhs, w * rhs);
}

Quat Quat::DivF32(f32 rhs) const
{
	f32 invRhs = 1.0 / rhs;
	return Quat(x * invRhs, y * invRhs, z * invRhs, w * invRhs);
}

Vec3 Quat::EulerAngles() const
{
	Vec3 euler;
	f32 sinrCosp = 2.0 * (w * x + y * z);
	f32 cosrCosp = 1.0 - 2.0 * (x * x + y * y);
	euler.x = Math::Degrees(std::atan2(sinrCosp, cosrCosp));
	f32 sinp = 2.0 * (w * y - z * x);
	if (std::abs(sinp) >= 1.0)
		euler.y = Math::Degrees(std::copysignf(Math::PI_2, sinp));
	else
		euler.y = Math::Degrees(std::asin(sinp));
	f32 sinyCosp = 2.0 * (w * z + x * y);
	f32 cosyCosp = 1.0 - 2.0 * (y * y + z * z);
	euler.z = Math::Degrees(std::atan2f(sinyCosp, cosyCosp));
	return euler;
}

Quat Quat::Euler(f32 x, f32 y, f32 z)
{
	Vec3 halfEuler = Vec3(x, y, z) * 0.5;
	f32 cr = cosf(Math::Radians(halfEuler.x));
	f32 sr = sinf(Math::Radians(halfEuler.x));
	f32 cy = cosf(Math::Radians(halfEuler.z));
	f32 sy = sinf(Math::Radians(halfEuler.z));
	f32 cp = cosf(Math::Radians(halfEuler.y));
	f32 sp = sinf(Math::Radians(halfEuler.y));
	return Quat(
		sr * cp * cy - cr * sp * sy,
		cr * sp * cy + sr * cp * sy,
		cr * cp * sy - sr * sp * cy,
		cr * cp * cy + sr * sp * sy
	);
}

Quat Quat::AngleAxis(f32 angleInDegrees, const Vec3& axis)
{
	f32 rangle = Math::Radians(angleInDegrees);
	f32 sha = sinf(rangle * 0.5);
	return Quat(axis.x * sha, axis.y * sha, axis.z * sha, cosf(rangle * 0.5));
}

f32 Quat::Dot(const Quat& a, const Quat& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

Quat Quat::Slerp(const Quat& a, const Quat& b, f32 t)
{
	Quat z = b;

	f32 cosTheta = Dot(a, b);
	if (cosTheta < 0)
	{
		z = -b;
		cosTheta = -cosTheta;
	}

	if (cosTheta > 1 - SAFE_DIV_EPSILON)
	{
		return Quat(
			Math::Lerp(a.x, z.x, t),
			Math::Lerp(a.y, z.y, t),
			Math::Lerp(a.z, z.z, t),
			Math::Lerp(a.w, z.w, t)
		);
	}
	else
	{
		f32 angle = acosf(cosTheta);
		return (a * sin((1.0 - t) * angle) + z * sin(t * angle)) / sinf(angle);
	}
}

Quat Quat::FromMat4(const Mat4& _matrix)
{
	glm::mat4 matrix = glm::make_mat4(_matrix.data);
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(matrix, scale, rotation, translation, skew, perspective);
	return QuatFromGLM(rotation);

	// TODO: why does this break the gauntlet game?? (used in Mat4::Decompose)
	// Source: https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/#google_vignette
	/*Quat q;
	Mat4 a = _matrix.Transpose();
	float trace = a(0, 0) + a(1, 1) + a(2, 2);
	if (trace > 0.0)
	{
		float s = 0.5f / sqrtf(trace + 1.0f);
		q.w = 0.25f / s;
		q.x = (a(2, 1) - a(1, 2)) * s;
		q.y = (a(0, 2) - a(2, 0)) * s;
		q.z = (a(1, 0) - a(0, 1)) * s;
	}
	else
	{
		if (a(0, 0) > a(1, 1) && a(0, 0) > a(2, 2))
		{
			float s = 2.0f * sqrtf(1.0f + a(0, 0) - a(1, 1) - a(2, 2));
			q.w = (a(2, 1) - a(1, 2)) / s;
			q.x = 0.25f * s;
			q.y = (a(0, 1) + a(1, 0)) / s;
			q.z = (a(0, 2) + a(2, 0)) / s;
		}
		else if (a(1, 1) > a(2, 2))
		{
			float s = 2.0f * sqrtf(1.0f + a(1, 1) - a(0, 0) - a(2, 2));
			q.w = (a(0, 2) - a(2, 0)) / s;
			q.x = (a(0, 1) + a(1, 0)) / s;
			q.y = 0.25f * s;
			q.z = (a(1, 2) + a(2, 1)) / s;
		}
		else
		{
			float s = 2.0f * sqrtf(1.0f + a(2, 2) - a(0, 0) - a(1, 1));
			q.w = (a(1, 0) - a(0, 1)) / s;
			q.x = (a(0, 2) + a(2, 0)) / s;
			q.y = (a(1, 2) + a(2, 1)) / s;
			q.z = 0.25f * s;
		}
	}
	return q;*/
}

Quat Quat::FromValuePtr(f32* vptr)
{
	Quat q;
	memcpy(q.data, vptr, sizeof(Quat));
	return q;
}

f32 Mat4::At(i32 i) const
{
	return data[i];
}

f32 Mat4::At(i32 i, i32 j) const
{
	return basis[i][j];
}

Mat4 Mat4::Mul(const Mat4& rhs) const
{
	Mat4 result;
	for (u32 y = 0; y < 4; y++)
	{
		for (u32 x = 0; x < 4; x++)
		{
			f64 sum = 0.0;
			for (u32 e = 0; e < 4; e++)
				sum += data[x + e * 4] * rhs[e + y * 4];
			result[x + y * 4] = sum;
		}
	}
	return result;
}

Vec4 Mat4::MulVec4(const Vec4& rhs) const
{
	return Vec4(this->basis[0].x * rhs.x + this->basis[1].x * rhs.y +
		this->basis[2].x * rhs.z + this->basis[3].x * rhs.w,
		this->basis[0].y * rhs.x + this->basis[1].y * rhs.y +
		this->basis[2].y * rhs.z + this->basis[3].y * rhs.w,
		this->basis[0].z * rhs.x + this->basis[1].z * rhs.y +
		this->basis[2].z * rhs.z + this->basis[3].z * rhs.w,
		this->basis[0].w * rhs.x + this->basis[1].w * rhs.y +
		this->basis[2].w * rhs.z + this->basis[3].w * rhs.w);
}

Mat4 Mat4::Transpose() const
{
	Mat4 result;
	for (i32 i = 0; i < 4; i++)
	{
		for (i32 j = 0; j < 4; j++)
		{
			result(i, j) = this->At(j, i);
		}
	}
	return result;
}

Mat4 Mat4::Inverse() const
{
	const f32 inv[16] = { data[5] * data[10] * data[15] -
								data[5] * data[11] * data[14] -
								data[9] * data[6] * data[15] +
								data[9] * data[7] * data[14] +
								data[13] * data[6] * data[11] -
								data[13] * data[7] * data[10],
							-data[1] * data[10] * data[15] +
								data[1] * data[11] * data[14] +
								data[9] * data[2] * data[15] -
								data[9] * data[3] * data[14] -
								data[13] * data[2] * data[11] +
								data[13] * data[3] * data[10],
							data[1] * data[6] * data[15] -
								data[1] * data[7] * data[14] -
								data[5] * data[2] * data[15] +
								data[5] * data[3] * data[14] +
								data[13] * data[2] * data[7] -
								data[13] * data[3] * data[6],
							-data[1] * data[6] * data[11] +
								data[1] * data[7] * data[10] +
								data[5] * data[2] * data[11] -
								data[5] * data[3] * data[10] -
								data[9] * data[2] * data[7] +
								data[9] * data[3] * data[6],
							-data[4] * data[10] * data[15] +
								data[4] * data[11] * data[14] +
								data[8] * data[6] * data[15] -
								data[8] * data[7] * data[14] -
								data[12] * data[6] * data[11] +
								data[12] * data[7] * data[10],
							data[0] * data[10] * data[15] -
								data[0] * data[11] * data[14] -
								data[8] * data[2] * data[15] +
								data[8] * data[3] * data[14] +
								data[12] * data[2] * data[11] -
								data[12] * data[3] * data[10],
							-data[0] * data[6] * data[15] +
								data[0] * data[7] * data[14] +
								data[4] * data[2] * data[15] -
								data[4] * data[3] * data[14] -
								data[12] * data[2] * data[7] +
								data[12] * data[3] * data[6],
							data[0] * data[6] * data[11] -
								data[0] * data[7] * data[10] -
								data[4] * data[2] * data[11] +
								data[4] * data[3] * data[10] +
								data[8] * data[2] * data[7] -
								data[8] * data[3] * data[6],
							data[4] * data[9] * data[15] -
								data[4] * data[11] * data[13] -
								data[8] * data[5] * data[15] +
								data[8] * data[7] * data[13] +
								data[12] * data[5] * data[11] -
								data[12] * data[7] * data[9],
							-data[0] * data[9] * data[15] +
								data[0] * data[11] * data[13] +
								data[8] * data[1] * data[15] -
								data[8] * data[3] * data[13] -
								data[12] * data[1] * data[11] +
								data[12] * data[3] * data[9],
							data[0] * data[5] * data[15] -
								data[0] * data[7] * data[13] -
								data[4] * data[1] * data[15] +
								data[4] * data[3] * data[13] +
								data[12] * data[1] * data[7] -
								data[12] * data[3] * data[5],
							-data[0] * data[5] * data[11] +
								data[0] * data[7] * data[9] +
								data[4] * data[1] * data[11] -
								data[4] * data[3] * data[9] -
								data[8] * data[1] * data[7] +
								data[8] * data[3] * data[5],
							-data[4] * data[9] * data[14] +
								data[4] * data[10] * data[13] +
								data[8] * data[5] * data[14] -
								data[8] * data[6] * data[13] -
								data[12] * data[5] * data[10] +
								data[12] * data[6] * data[9],
							data[0] * data[9] * data[14] -
								data[0] * data[10] * data[13] -
								data[8] * data[1] * data[14] +
								data[8] * data[2] * data[13] +
								data[12] * data[1] * data[10] -
								data[12] * data[2] * data[9],
							-data[0] * data[5] * data[14] +
								data[0] * data[6] * data[13] +
								data[4] * data[1] * data[14] -
								data[4] * data[2] * data[13] -
								data[12] * data[1] * data[6] +
								data[12] * data[2] * data[5],
							data[0] * data[5] * data[10] -
								data[0] * data[6] * data[9] -
								data[4] * data[1] * data[10] +
								data[4] * data[2] * data[9] +
								data[8] * data[1] * data[6] -
								data[8] * data[2] * data[5] };

	Mat4 result = *this;
	const f32 det = data[0] * inv[0] + data[1] * inv[4] +
		data[2] * inv[8] + data[3] * inv[12];
	if (det != 0.0)
	{
		const f32 invdet = 1.0 / det;
		for (u32 i = 0; i < 16; i++)
		{
			result[i] = inv[i] * invdet;
		}
	}
	return result;
}

Mat4 Mat4::Identity()
{
	static Mat4 s_identity = Mat4(
		Vec4(1, 0, 0, 0),
		Vec4(0, 1, 0, 0),
		Vec4(0, 0, 1, 0),
		Vec4(0, 0, 0, 1)
	);
	return s_identity;
}

Mat4 Mat4::Zero()
{
	return Mat4(
		Vec4(0, 0, 0, 0),
		Vec4(0, 0, 0, 0),
		Vec4(0, 0, 0, 0),
		Vec4(0, 0, 0, 0)
	);
}

Mat4 Mat4::LookAt(const Vec3& eye, const Vec3& center, const Vec3& up)
{
	glm::vec3 e = glm::make_vec3(eye.data);
	glm::vec3 c = glm::make_vec3(center.data);
	glm::vec3 u = glm::make_vec3(up.data);
	glm::mat4 m = glm::lookAt(e, c, u);
	return Mat4::FromValuePtr(glm::value_ptr(m));
}

Mat4 Mat4::Ortho(f32 left, f32 right, f32 bottom, f32 top, f32 zNear, f32 zFar)
{
	Mat4 result = Mat4::Identity();
	result(0, 0) = 2.0 / (right - left);
	result(1, 1) = 2.0 / (top - bottom);
	result(2, 2) = -2.0 / (zFar - zNear);
	result(3, 0) = (left + right) / (right - left);
	result(3, 1) = (bottom + top) / (top - bottom);
	result(3, 2) = -(zFar + zNear) / (zFar - zNear);
	return result;
}

Mat4 Mat4::Perspective(f32 fov, f32 aspect, f32 zNear, f32 zFar)
{
	Mat4 result = Mat4::Identity();
	f32 q = 1.0 / tanf(Math::Radians(0.5 * fov));
	f32 a = q / aspect;
	f32 b = (zNear + zFar) / (zNear - zFar);
	f32 c = (2.0f * zNear * zFar) / (zNear - zFar);
	result(0, 0) = a;
	result(1, 1) = q;
	result(2, 2) = b;
	result(2, 3) = -1.0;
	result(3, 2) = c;
	return result;
}

Mat4 Mat4::Translation(const Vec3& translation)
{
	Mat4 result = Mat4::Identity();
	result(3, 0) = translation.x;
	result(3, 1) = translation.y;
	result(3, 2) = translation.z;
	return result;
}

Mat4 Mat4::Rotation(f32 angle, const Vec3& axis)
{
	Mat4 result = Mat4::Identity();
	f32 r = Math::Radians(angle);
	f32 c = cosf(r);
	f32 s = sinf(r);
	f32 omc = 1.0 - c;
	result(0, 0) = axis.x * omc + c;
	result(0, 1) = axis.y * axis.x * omc + axis.z * s;
	result(0, 2) = axis.z * axis.x * omc - axis.y * s;
	result(1, 0) = axis.x * axis.y * omc - axis.z * s;
	result(1, 1) = axis.y * omc + c;
	result(1, 2) = axis.y * axis.z * omc + axis.x * s;
	result(2, 0) = axis.x * axis.z * omc + axis.y * s;
	result(2, 1) = axis.y * axis.z * omc - axis.x * s;
	result(2, 2) = axis.z * omc + c;
	return result;
}

Mat4 Mat4::Rotation(const Quat& rotation)
{
	Mat4 result = Mat4::Identity();
	result[0] = 1.0 - 2.0 * rotation.y * rotation.y - 2.0 * rotation.z * rotation.z;
	result[4] = 2.0 * rotation.x * rotation.y - 2.0 * rotation.w * rotation.z;
	result[8] = 2.0 * rotation.x * rotation.z + 2.0 * rotation.w * rotation.y;
	result[1] = 2.0 * rotation.x * rotation.y + 2.0 * rotation.w * rotation.z;
	result[5] = 1.0 - 2.0 * rotation.x * rotation.x - 2.0 * rotation.z * rotation.z;
	result[9] = 2.0 * rotation.y * rotation.z - 2.0 * rotation.w * rotation.x;
	result[2] = 2.0 * rotation.x * rotation.z - 2.0 * rotation.w * rotation.y;
	result[6] = 2.0 * rotation.y * rotation.z + 2.0 * rotation.w * rotation.x;
	result[10] = 1.0 - 2.0 * rotation.x * rotation.x - 2.0 * rotation.y * rotation.y;
	return result;
}

Mat4 Mat4::Scale(const Vec3& scale)
{
	Mat4 result = Mat4::Identity();
	result(0, 0) = scale.x;
	result(1, 1) = scale.y;
	result(2, 2) = scale.z;
	return result;
}

Mat4 Mat4::TRS(const Vec3& translation, const Quat& rotation, const Vec3& scale)
{
	return Mat4::Translation(translation) * Mat4::Rotation(rotation) * Mat4::Scale(scale);
}

void Mat4::Decompose(const Mat4& matrix, Vec3& pos, Quat& rot, Vec3& scl)
{
	pos = Vec3(matrix.basis[3][0], matrix.basis[3][1], matrix.basis[3][2]);
	rot = Quat::FromMat4(matrix);
	scl = Vec3(
		Vec3(matrix.basis[0][0], matrix.basis[0][1], matrix.basis[0][2]).Magnitude(),
		Vec3(matrix.basis[1][0], matrix.basis[1][1], matrix.basis[1][2]).Magnitude(),
		Vec3(matrix.basis[2][0], matrix.basis[2][1], matrix.basis[2][2]).Magnitude()
	);
}

Mat4 Mat4::FromValuePtr(f32* vptr)
{
	Mat4 m;
	memcpy(m.data, vptr, sizeof(Mat4));
	return m;
}

#endif // USE_GLM_IMPL

namespace Packing
{
	u32 Pack2xF16(f16 data[2])
	{
		return static_cast<u32>(data[0].data) | (static_cast<u32>(data[1].data) >> 16);
	}

	u32 Pack4xU8(u8 data[4])
	{
		return (u32)data[0] >> 24 | (u32)data[1] >> 16 | (u32)data[2] >> 8 | (u32)data[3];
	}
}

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/PixelPacking_RGBE.hlsli
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
PackedRgb9e5::PackedRgb9e5(Vec3 rgb)
{
	using namespace Math;

	float max_val = U32BitsToF32(0x477F8000u);
	float min_val = U32BitsToF32(0x37800000u);
	Vec3 clamped_rgb = Vec3::Clamp(rgb, Vec3::Splat(0.0f), Vec3::Splat(max_val));
	float max_channel = Max(Max(min_val, clamped_rgb.x), Max(clamped_rgb.y, clamped_rgb.z));
	float bias = U32BitsToF32((F32BitsToU32(max_channel) + 0x07804000u) & 0x7F800000u);
	rgb = clamped_rgb + bias;
	Vec3u rgbui = Vec3u(
		F32BitsToU32(rgb.x),
		F32BitsToU32(rgb.y),
		F32BitsToU32(rgb.z)
	);
	u32 e = (F32BitsToU32(bias) << 4u) + 0x10000000u;
	data = e | rgbui.z << 18 | rgbui.y << 9 | (rgbui.x & 0x1FFu);
}

// https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/PixelPacking_RGBE.hlsli
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
Vec3 PackedRgb9e5::Unpack() const
{
	Vec3 rgb = Vec3(data & 0x1FFu, (data >> 9) & 0x1FFu, (data >> 18) & 0x1FFu);
	u32 l = (data >> 27) - 24;
	return Vec3(
		std::ldexp(rgb.x, l),
		std::ldexp(rgb.y, l),
		std::ldexp(rgb.z, l)
	);
}

// Inspired by https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
Vec2 DirOctQuadEncode(Vec3 dir)
{
	Vec2 ret_val = dir.Xy() / (abs(dir.x) + abs(dir.y) + abs(dir.z));
	if (dir.z < 0.0f) {
		Vec2 signs = Vec2(ret_val.x >= 0.0f ? 1.0f : -1.0f, ret_val.y >= 0.0f ? 1.0f : -1.0f);
		ret_val = (Vec2::Splat(1.0) - ret_val.Yx().Abs()) * signs;
	}
	return ret_val * 0.5 + 0.5;
}

// Inspired by https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
Vec3 DirOctQuadDecode(Vec2 encoded_in)
{
	Vec2 encoded = encoded_in * 2.0 - 1.0;
	Vec3 n = Vec3(encoded.x, encoded.y, 1.0 - abs(encoded.x) - abs(encoded.y));
	float t = Math::Clamp(-n.z, 0.0f, 1.0f);
	Vec2 added = Vec2(n.x >= 0.0 ? -t : t, n.y >= 0.0 ? -t : t);
	n.x += added.x;
	n.y += added.y;
	return n.Normalized();
}

u32 Pack30OctEncodedDir(Vec2 oct_encoded_dir, u32 offset)
{
	return ((u32(round(oct_encoded_dir.y * float(0x7fff))) << 15) |
		u32(round(oct_encoded_dir.x * float(0x7fff))))
		<< offset;
}

Vec2 Unpack30OctEncodedDir(u32 packed, u32 offset)
{
	return Vec2(float((packed >> offset) & 0x7fff) / float(0x7fff),
		float((packed >> (offset + 15)) & 0x7fff) / float(0x7fff));
}

PackedNormalizedXyz10::PackedNormalizedXyz10(Vec3 dir, u32 offset)
{
	Vec2 oct_encoded_dir = DirOctQuadEncode(dir);
	data = Pack30OctEncodedDir(oct_encoded_dir, offset);
}

Vec3 PackedNormalizedXyz10::Unpack(u32 offset) const
{
	return DirOctQuadDecode(Unpack30OctEncodedDir(data, offset));
}