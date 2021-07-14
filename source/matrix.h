
struct float4x4
{
#pragma warning(disable: 4201)
	// Layout is column major, so m is an array of columns
	union 
	{
		float m[4][4];
		struct 
		{
			float m00, m10, m20, m30;
			float m01, m11, m21, m31;
			float m02, m12, m22, m32;
			float m03, m13, m23, m33;
		};
	};
#pragma warning(default: 4201)

	float4x4()
	{
		ZeroMemory(this, sizeof(*this));
	}

	float4x4(float diagonal)
	{
		ZeroMemory(this, sizeof(*this));
		m00 = m11 = m22 = m33 = diagonal;
	}

	explicit float4x4(
		float a00, float a01, float a02, float a03,
		float a10, float a11, float a12, float a13,
		float a20, float a21, float a22, float a23,
		float a30, float a31, float a32, float a33)
	{
		m00 = a00; m01 = a01; m02 = a02; m03 = a03;
		m10 = a10; m11 = a11; m12 = a12; m13 = a13;
		m20 = a20; m21 = a21; m22 = a22; m23 = a23;
		m30 = a30; m31 = a31; m32 = a32; m33 = a33;
	}
};

inline float3 operator*(const float4x4& f, const float3& vec)
{
	return float3 {
		f.m00*vec.x + f.m01*vec.y + f.m02*vec.z + f.m03,
		f.m10*vec.x + f.m11*vec.y + f.m12*vec.z + f.m13,
		f.m20*vec.x + f.m21*vec.y + f.m22*vec.z + f.m23
	};
}

inline float4 operator*(const float4x4& f, const float4& vec)
{
	return float4 {
		f.m00*vec.x + f.m01*vec.y + f.m02*vec.z + f.m03*vec.w,
		f.m10*vec.x + f.m11*vec.y + f.m12*vec.z + f.m13*vec.w,
		f.m20*vec.x + f.m21*vec.y + f.m22*vec.z + f.m23*vec.w,
		f.m30*vec.x + f.m31*vec.y + f.m32*vec.z + f.m33*vec.w,
	};
}

inline float4x4 operator*(const float4x4& lhs, const float4x4& rhs)
{
	float4x4 res;

	res.m00 = lhs.m00*rhs.m00 + lhs.m01*rhs.m10 + lhs.m02*rhs.m20 + lhs.m03*rhs.m30;
	res.m10 = lhs.m10*rhs.m00 + lhs.m11*rhs.m10 + lhs.m12*rhs.m20 + lhs.m13*rhs.m30;
	res.m20 = lhs.m20*rhs.m00 + lhs.m21*rhs.m10 + lhs.m22*rhs.m20 + lhs.m23*rhs.m30;
	res.m30 = lhs.m30*rhs.m00 + lhs.m31*rhs.m10 + lhs.m32*rhs.m20 + lhs.m33*rhs.m30;

	res.m01 = lhs.m00*rhs.m01 + lhs.m01*rhs.m11 + lhs.m02*rhs.m21 + lhs.m03*rhs.m31;
	res.m11 = lhs.m10*rhs.m01 + lhs.m11*rhs.m11 + lhs.m12*rhs.m21 + lhs.m13*rhs.m31;
	res.m21 = lhs.m20*rhs.m01 + lhs.m21*rhs.m11 + lhs.m22*rhs.m21 + lhs.m23*rhs.m31;
	res.m31 = lhs.m30*rhs.m01 + lhs.m31*rhs.m11 + lhs.m32*rhs.m21 + lhs.m33*rhs.m31;

	res.m02 = lhs.m00*rhs.m02 + lhs.m01*rhs.m12 + lhs.m02*rhs.m22 + lhs.m03*rhs.m32;
	res.m12 = lhs.m10*rhs.m02 + lhs.m11*rhs.m12 + lhs.m12*rhs.m22 + lhs.m13*rhs.m32;
	res.m22 = lhs.m20*rhs.m02 + lhs.m21*rhs.m12 + lhs.m22*rhs.m22 + lhs.m23*rhs.m32;
	res.m32 = lhs.m30*rhs.m02 + lhs.m31*rhs.m12 + lhs.m32*rhs.m22 + lhs.m33*rhs.m32;

	res.m03 = lhs.m00*rhs.m03 + lhs.m01*rhs.m13 + lhs.m02*rhs.m23 + lhs.m03*rhs.m33;
	res.m13 = lhs.m10*rhs.m03 + lhs.m11*rhs.m13 + lhs.m12*rhs.m23 + lhs.m13*rhs.m33;
	res.m23 = lhs.m20*rhs.m03 + lhs.m21*rhs.m13 + lhs.m22*rhs.m23 + lhs.m23*rhs.m33;
	res.m33 = lhs.m30*rhs.m03 + lhs.m31*rhs.m13 + lhs.m32*rhs.m23 + lhs.m33*rhs.m33;

	return res;
}

inline void transpose(float4x4& f)
{
	std::swap(f.m01, f.m10);
	std::swap(f.m02, f.m20);
	std::swap(f.m03, f.m30);
	std::swap(f.m12, f.m21);
	std::swap(f.m13, f.m31);
	std::swap(f.m23, f.m32);
}

inline float4x4 transposed(const float4x4& f)
{
	return float4x4(
		f.m00, f.m10, f.m20, f.m30,
		f.m01, f.m11, f.m21, f.m31,
		f.m02, f.m12, f.m22, f.m32,
		f.m03, f.m13, f.m23, f.m33
	);
}

// http://msdn.microsoft.com/en-us/library/windows/desktop/bb205342(v=vs.85).aspx
float4x4 lookAt(const float3& from, const float3& to)
{
	float3 up = { 0.f, 1.f, 0.f };
	float3 zaxis = normalized(to-from);
	float3 xaxis = normalized(cross(up, zaxis));
	float3 yaxis = cross(zaxis,xaxis);

	float4x4 m = float4x4(
		xaxis.x,	xaxis.y,	xaxis.z,	-dot(xaxis, from),
		yaxis.x,	yaxis.y,	yaxis.z,	-dot(yaxis, from),
		zaxis.x,	zaxis.y,	zaxis.z,	-dot(zaxis, from),
		0.0f,		0.0f,		0.0f,		1.0f);
	return m;
}

//https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh?redirectedfrom=MSDN
float4x4 projection(float fovv, float aspect, float zn, float zf)
{
	float yScale = 1.f / std::tan(fovv*0.5f);
	float xScale = yScale / aspect;

	float4x4 m = float4x4(
		xScale,		0.f,	0.f,		0.f,
		0.f,		yScale,	0.f,		0.f,
		0.f,		0.f,	zf/(zf-zn),	-zn*zf/(zf-zn),
		0.f,		0.f,	1,			0.f);
	return m;
}
