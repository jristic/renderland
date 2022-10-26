
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
	void* Mem = Allocate(Alloc, sizeof(T));
	T* Obj = (T*)Mem;
	ZeroMemory(Obj, sizeof(T));
	return Obj;
}

}
}
