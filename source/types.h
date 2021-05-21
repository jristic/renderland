
typedef unsigned int u32;
typedef signed int i32;
static_assert(sizeof(u32) == 4, "Didn't get expected size.");
static_assert(sizeof(i32) == 4, "Didn't get expected size.");
typedef unsigned short u16;
typedef signed short i16;
static_assert(sizeof(u16) == 2, "Didn't get expected size.");
static_assert(sizeof(i16) == 2, "Didn't get expected size.");

struct uint2 {
	u32 x,y;
};
struct uint3 {
	u32 x,y,z;
};

struct float4 {
	float x,y,z,w;
};
