
typedef unsigned int u32;
typedef signed int i32;
static_assert(sizeof(u32) == 4, "Didn't get expected size.");
static_assert(sizeof(i32) == 4, "Didn't get expected size.");
typedef unsigned short u16;
typedef signed short i16;
static_assert(sizeof(u16) == 2, "Didn't get expected size.");
static_assert(sizeof(i16) == 2, "Didn't get expected size.");
typedef unsigned char u8;
typedef signed char i8;
static_assert(sizeof(u8) == 1, "Didn't get expected size.");
static_assert(sizeof(i8) == 1, "Didn't get expected size.");

#pragma warning(disable: 4201)

struct bool4 {
	union {
		bool m[4];
		struct {
			bool x,y,z,w;
		};
	};
};

struct int2 {
	i32 x,y;
};
struct int3 {
	i32 x,y,z;
};
struct int4 {
	union {
		i32 m[4];
		struct {
			i32 x,y,z,w;
		};
	};
};

struct uint2 {
	u32 x,y;
};
struct uint3 {
	u32 x,y,z;
};
struct uint4 {
	union {
		u32 m[4];
		struct {
			u32 x,y,z,w;
		};
	};
};

struct float2 {
	float x,y;
};
struct float3 {
	float x,y,z;
};
struct float4 {
	union {
		float m[4];
		struct {
			float x,y,z,w;
		};
	};
	float& operator[](u32 i) { return m[i]; }
	float operator[](u32 i) const { return m[i]; }
};

#pragma warning(default: 4201)
