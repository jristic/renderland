
namespace rlf {
namespace alloc {


void Init(LinAlloc* Alloc)
{
	Assert(Alloc->CurrentPage == nullptr, "Alloc misuse.");
	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);
	Alloc->PageSize = SysInfo.dwPageSize;
	Alloc->CurrentPage = (u8*)VirtualAlloc(
		nullptr, 
		Alloc->PageSize,
		MEM_COMMIT,
		PAGE_READWRITE
	);
	*(u8**)(Alloc->CurrentPage) = nullptr;
	Alloc->CurrentOffset = sizeof(u8*);
}

void* Allocate(LinAlloc* Alloc, u32 AllocSize)
{
	Assert(AllocSize < Alloc->PageSize, "Allocation too large.");
	if (Alloc->PageSize - Alloc->CurrentOffset < AllocSize) {
		void* Address = Alloc->CurrentPage + Alloc->CurrentOffset;
		Alloc->CurrentOffset += AllocSize;
		return Address;
	}
	u8* LastPage = Alloc->CurrentPage;
	Alloc->CurrentPage = (u8*)VirtualAlloc(
		nullptr, 
		Alloc->PageSize,
		MEM_COMMIT,
		PAGE_READWRITE
	);
	*(u8**)(Alloc->CurrentPage) = LastPage;
	Alloc->CurrentOffset = sizeof(u8*);
	return Alloc->CurrentPage + Alloc->CurrentOffset;
}

void FreeAll(LinAlloc* Alloc)
{
	u8* NextPage = Alloc->CurrentPage;
	while (NextPage) {
		void* ReleaseAddress = NextPage;
		NextPage = *(u8**)NextPage;
		VirtualFree(
			ReleaseAddress,
			Alloc->PageSize,
			MEM_RELEASE
		);
	}
}


} // namespace alloc
} // namespace rlf
