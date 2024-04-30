
namespace rlf {
namespace alloc {


struct LinAlloc {
	u8* CurrentPage;
	u32 CurrentOffset;
	u32 PageSize;
};

void Init(LinAlloc* Alloc);
void* Allocate(LinAlloc* Alloc, u32 AllocSize);
void FreeAll(LinAlloc* Alloc);


template <typename T>
T* Allocate(LinAlloc* Alloc)
{
	return (T*)Allocate(Alloc, sizeof(T));
}

template <typename T>
Array<T> MakeCopy(LinAlloc* Alloc, std::vector<T> Source)
{
	Array<T> Dest;
	Assert(Source.size() < U32_MAX, "array too big");
	Dest.Count = (u32)Source.size();
	Dest.Data = (T*)Allocate(Alloc, Dest.Count * sizeof(T));
	memcpy(Dest.Data, Source.data(), Dest.Count * sizeof(T));

	return Dest;
}


}
}