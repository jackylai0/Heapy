#include <windows.h>
#include <iostream>
#include <malloc.h>

void MallocLeakyFunction(){
	malloc(1024*1024*5); // leak 5Mb
}

void MallocNonLeakyFunction(){
	auto p = malloc(1024*1024); // allocate 1Mb
	std::cout << "TestApplication: Sleeping..." << std::endl;
	Sleep(15000);
	free(p); // free the Mb
}

void CallocLeakyFunction(){
	calloc(1024, 1024*5); // leak 5Mb
}

void CallocNonLeakyFunction(){
	auto p = calloc(1024, 1024); // allocate 1Mb
	std::cout << "TestApplication: Sleeping..." << std::endl;
	Sleep(15000);
	free(p); // free the Mb
}

void ReallocLeakyFunction(){
	auto p = realloc(NULL, 1024*1024*1); // allocate 1Mb
	auto p2 = realloc(p, 1024*1024*5); // reallocate and leak 5Mb
}

void ReallocNonLeakyFunction(){
	auto p = realloc(NULL, 1024*1024); // allocate 1Mb
	std::cout << "TestApplication: Sleeping..." << std::endl;
	Sleep(15000);
	auto p2 = realloc(p, 1024*1024*2); // reallocate 2Mb
	if (p2 != NULL)
		p = p2;
	Sleep(15000);
	realloc(p, 0); // free the Mb (as specified by CRT reference)
}

void RecallocLeakyFunction(){
	auto p = _recalloc(NULL, 1024, 1024*1); // allocate 1Mb
	auto p2 = _recalloc(p, 1024, 1024*5); // reallocate and leak 5Mb
}

void RecallocNonLeakyFunction(){
	auto p = _recalloc(NULL, 1024, 1024); // allocate 1Mb
	std::cout << "TestApplication: Sleeping..." << std::endl;
	Sleep(15000);
	auto p2 = _recalloc(p, 1024, 1024*2); // reallocate 2Mb
	if (p2 != NULL)
		p = p2;
	Sleep(15000);
	realloc(p, 0); // free the Mb (as specified by CRT reference)
}

void AlignedMallocLeakyFunction(){
	_aligned_malloc(1024*1024*5, 16); // leak 5Mb
}

void AlignedMallocNonLeakyFunction(){
	auto p = _aligned_malloc(1024*1024, 16); // allocate 1Mb
	std::cout << "TestApplication: Sleeping..." << std::endl;
	Sleep(15000);
	_aligned_free(p); // free the Mb
}

void AlignedReallocLeakyFunction(){
	auto p = _aligned_realloc(NULL, 1024*1024*1, 16); // allocate 1Mb
	auto p2 = _aligned_realloc(p, 1024*1024*5, 16); // reallocate and leak 5Mb
}

void AlignedReallocNonLeakyFunction(){
	auto p = _aligned_realloc(NULL, 1024*1024, 16); // allocate 1Mb
	std::cout << "TestApplication: Sleeping..." << std::endl;
	Sleep(15000);
	auto p2 = _aligned_realloc(p, 1024*1024*2, 16); // reallocate 2Mb
	if (p2 != NULL)
		p = p2;
	Sleep(15000);
	_aligned_realloc(p, 0, 16); // free the Mb (as specified by CRT reference)
}

void AlignedRecallocLeakyFunction(){
	auto p = _aligned_recalloc(NULL, 1024, 1024*1, 16); // allocate 1Mb
	auto p2 = _aligned_recalloc(p, 1024, 1024*5, 16); // reallocate and leak 5Mb
}

void AlignedRecallocNonLeakyFunction(){
	auto p = _aligned_recalloc(NULL, 1024, 1024, 16); // allocate 1Mb
	std::cout << "TestApplication: Sleeping..." << std::endl;
	Sleep(15000);
	auto p2 = _aligned_recalloc(p, 1024, 1024*2, 16); // reallocate 2Mb
	if (p2 != NULL)
		p = p2;
	Sleep(15000);
	_aligned_recalloc(p, 0, 0, 16); // free the Mb (as specified by CRT reference)
}

void AlignedOffsetMallocLeakyFunction(){
	_aligned_offset_malloc(1024*1024*5, 16, 16); // leak 5Mb
}

void AlignedOffsetMallocNonLeakyFunction(){
	auto p = _aligned_offset_malloc(1024*1024, 16, 16); // allocate 1Mb
	std::cout << "TestApplication: Sleeping..." << std::endl;
	Sleep(15000);
	_aligned_free(p); // free the Mb
}

void AlignedOffsetReallocLeakyFunction(){
	auto p = _aligned_offset_realloc(NULL, 1024*1024*1, 16, 16); // allocate 1Mb
	auto p2 = _aligned_offset_realloc(p, 1024*1024*5, 16, 16); // reallocate and leak 5Mb
}

void AlignedOffsetReallocNonLeakyFunction(){
	auto p = _aligned_offset_realloc(NULL, 1024*1024, 16, 16); // allocate 1Mb
	std::cout << "TestApplication: Sleeping..." << std::endl;
	Sleep(15000);
	auto p2 = _aligned_offset_realloc(p, 1024*1024*2, 16, 16); // reallocate 2Mb
	if (p2 != NULL)
		p = p2;
	Sleep(15000);
	_aligned_offset_realloc(p, 0, 16, 16); // free the Mb (as specified by CRT reference)
}

void AlignedOffsetRecallocLeakyFunction(){
	auto p = _aligned_offset_recalloc(NULL, 1024, 1024*1, 16, 16); // allocate 1Mb
	auto p2 = _aligned_offset_recalloc(p, 1024, 1024*5, 16, 16); // reallocate and leak 5Mb
}

void AlignedOffsetRecallocNonLeakyFunction(){
	auto p = _aligned_offset_recalloc(NULL, 1024, 1024, 16, 16); // allocate 1Mb
	std::cout << "TestApplication: Sleeping..." << std::endl;
	Sleep(15000);
	auto p2 = _aligned_offset_recalloc(p, 1024, 1024*2, 16, 16); // reallocate 2Mb
	if (p2 != NULL)
		p = p2;
	Sleep(15000);
	_aligned_offset_recalloc(p, 0, 0, 16, 16); // free the Mb (as specified by CRT reference)
}

int main()
{
	std::cout << "TestApplication: Creating some leaks..." << std::endl;
	for(int i = 0; i < 5; ++i){
		MallocLeakyFunction();
		CallocLeakyFunction();
		ReallocLeakyFunction();
		RecallocLeakyFunction();
		AlignedMallocLeakyFunction();
		AlignedReallocLeakyFunction();
		AlignedRecallocLeakyFunction();
		AlignedOffsetMallocLeakyFunction();
		AlignedOffsetReallocLeakyFunction();
		AlignedOffsetRecallocLeakyFunction();
	}
	MallocNonLeakyFunction();
	CallocNonLeakyFunction();
	ReallocNonLeakyFunction();
	RecallocNonLeakyFunction();
	AlignedMallocNonLeakyFunction();
	AlignedReallocNonLeakyFunction();
	AlignedRecallocNonLeakyFunction();
	AlignedOffsetMallocNonLeakyFunction();
	AlignedOffsetReallocNonLeakyFunction();
	AlignedOffsetRecallocNonLeakyFunction();
	std::cout << "TestApplication: Exiting..." << std::endl;
	return 0;
}
