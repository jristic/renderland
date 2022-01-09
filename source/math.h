
float3 operator-(const float3& lhs, const float3& rhs)
{
	float3 res;
	res.x = lhs.x - rhs.x;
	res.y = lhs.y - rhs.y;
	res.z = lhs.z - rhs.z;
	return res;
}

float4 operator+(const float4& lhs, const float4& rhs)
{
	float4 res;
	res.x = lhs.x + rhs.x;
	res.y = lhs.y + rhs.y;
	res.z = lhs.z + rhs.z;
	res.w = lhs.w + rhs.w;
	return res;
}

float4 operator-(const float4& lhs, const float4& rhs)
{
	float4 res;
	res.x = lhs.x - rhs.x;
	res.y = lhs.y - rhs.y;
	res.z = lhs.z - rhs.z;
	res.w = lhs.w - rhs.w;
	return res;
}

float4 operator*(const float4& lhs, const float4& rhs)
{
	float4 res;
	res.x = lhs.x * rhs.x;
	res.y = lhs.y * rhs.y;
	res.z = lhs.z * rhs.z;
	res.w = lhs.w * rhs.w;
	return res;
}

float4 operator/(const float4& lhs, const float4& rhs)
{
	float4 res;
	res.x = lhs.x / rhs.x;
	res.y = lhs.y / rhs.y;
	res.z = lhs.z / rhs.z;
	res.w = lhs.w / rhs.w;
	return res;
}

int4 operator+(const int4& lhs, const int4& rhs)
{
	int4 res;
	res.x = lhs.x + rhs.x;
	res.y = lhs.y + rhs.y;
	res.z = lhs.z + rhs.z;
	res.w = lhs.w + rhs.w;
	return res;
}

int4 operator-(const int4& lhs, const int4& rhs)
{
	int4 res;
	res.x = lhs.x - rhs.x;
	res.y = lhs.y - rhs.y;
	res.z = lhs.z - rhs.z;
	res.w = lhs.w - rhs.w;
	return res;
}

int4 operator*(const int4& lhs, const int4& rhs)
{
	int4 res;
	res.x = lhs.x * rhs.x;
	res.y = lhs.y * rhs.y;
	res.z = lhs.z * rhs.z;
	res.w = lhs.w * rhs.w;
	return res;
}

int4 operator/(const int4& lhs, const int4& rhs)
{
	int4 res;
	res.x = lhs.x / rhs.x;
	res.y = lhs.y / rhs.y;
	res.z = lhs.z / rhs.z;
	res.w = lhs.w / rhs.w;
	return res;
}

uint4 operator+(const uint4& lhs, const uint4& rhs)
{
	uint4 res;
	res.x = lhs.x + rhs.x;
	res.y = lhs.y + rhs.y;
	res.z = lhs.z + rhs.z;
	res.w = lhs.w + rhs.w;
	return res;
}

uint4 operator-(const uint4& lhs, const uint4& rhs)
{
	uint4 res;
	res.x = lhs.x - rhs.x;
	res.y = lhs.y - rhs.y;
	res.z = lhs.z - rhs.z;
	res.w = lhs.w - rhs.w;
	return res;
}

uint4 operator*(const uint4& lhs, const uint4& rhs)
{
	uint4 res;
	res.x = lhs.x * rhs.x;
	res.y = lhs.y * rhs.y;
	res.z = lhs.z * rhs.z;
	res.w = lhs.w * rhs.w;
	return res;
}

uint4 operator/(const uint4& lhs, const uint4& rhs)
{
	uint4 res;
	res.x = lhs.x / rhs.x;
	res.y = lhs.y / rhs.y;
	res.z = lhs.z / rhs.z;
	res.w = lhs.w / rhs.w;
	return res;
}

bool operator==(const uint2& l, const uint2& r)
{
	return l.x == r.x && l.y == r.y;
}
bool operator!=(const uint2& l, const uint2& r)
{
	return l.x != r.x || l.y != r.y;
}

float3 normalized(const float3& a)
{
	float mag = std::sqrt(a.x*a.x + a.y*a.y + a.z*a.z);
	return float3 { a.x/mag, a.y/mag, a.z/mag };
}

float dot(const float3& a, const float3& b)
{
	float d = a.x*b.x + a.y*b.y + a.z*b.z;
	return d;
}

float3 cross(const float3& a, const float3& b)
{
	float3 c;
	c.x = a.y*b.z - a.z*b.y;
	c.y = a.z*b.x - a.x*b.z;
	c.z = a.x*b.y - a.y*b.x;
	return c;
}

float sqr(float x)
{
	return x*x;
}
