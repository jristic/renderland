
namespace rlf {
namespace alloc {


struct BlockHeader
{
	u8* PrevBlock;
	u32 NumPages;
};

void Init(LinAlloc* Alloc)
{
	Assert(Alloc->CurrentBlock == nullptr, "Alloc misuse.");
	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);
	Alloc->CurrentBlock = nullptr;
	Alloc->NextOffset = 0xffffffff;
	Alloc->PageSize = SysInfo.dwPageSize;
}

void* Allocate(LinAlloc* Alloc, u32 AllocSize)
{
	Assert(AllocSize > 0, "Invalid allocation.");
	BlockHeader* Header = (BlockHeader*)Alloc->CurrentBlock;
	if (Alloc->CurrentBlock != nullptr && 
		(Header->NumPages*Alloc->PageSize - Alloc->NextOffset >= AllocSize)) 
	{
		void* Address = Alloc->CurrentBlock + Alloc->NextOffset;
		Alloc->NextOffset += AllocSize;
		return Address;
	}
	u8* LastBlock = Alloc->CurrentBlock;
	u32 TotalSize = AllocSize + sizeof(BlockHeader);
	// Round up to next multiple of PageSize
	u32 NumPages = ((TotalSize - 1) / Alloc->PageSize) + 1;
	Alloc->CurrentBlock = (u8*)VirtualAlloc(
		nullptr, 
		Alloc->PageSize * NumPages,
		MEM_COMMIT,
		PAGE_READWRITE
	);
	Header = (BlockHeader*)Alloc->CurrentBlock;
	Header->PrevBlock = LastBlock;
	Header->NumPages = NumPages;
	Alloc->NextOffset = sizeof(BlockHeader) + AllocSize;
	return Alloc->CurrentBlock + sizeof(BlockHeader);
}

void* Allocate(LinAlloc* Alloc, size_t AllocSize)
{
	Assert(AllocSize < U32_MAX, "Allocation too large.");
	return Allocate(Alloc, (u32)AllocSize);
}


void FreeAll(LinAlloc* Alloc)
{
	BlockHeader* Header = (BlockHeader*)Alloc->CurrentBlock;
	while (Header) {
		void* ReleaseAddress = Header;
		u32 ReleaseSize = Alloc->PageSize * Header->NumPages;
		Header = (BlockHeader*)Header->PrevBlock;
		VirtualFree(
			ReleaseAddress,
			ReleaseSize,
			MEM_RELEASE
		);
	}
}


} // namespace alloc
} // namespace rlf
