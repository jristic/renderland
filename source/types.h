
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

struct float2 {
	float x,y;
};
struct float3 {
	float x,y,z;
};
struct float4 {
	float x,y,z,w;
};


template <std::size_t N>
struct type_of_size
{
    typedef char type[N];
};

template <typename T, std::size_t Size>
typename type_of_size<Size>::type& sizeof_array_helper(T(&)[Size]);

#define sizeof_array(pArray) sizeof(sizeof_array_helper(pArray))

