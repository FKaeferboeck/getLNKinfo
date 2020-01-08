#include "getLNKinfo.h"
#include <iostream>
#include <utility>

using std::wstring;

const WError errInLNK{ L"Linkfile is broken" };


wstring takeFilename(const wchar_t* pathFile) {
	const wchar_t* p = pathFile;
	wchar_t c;
	while(*p)     ++p;
	while(p > pathFile && (c = *(p - 1)) != L'/' && c != L'\\')     --p;
	return wstring{ p };
}


wstring takePathname(const wchar_t* pathFile) { // without trailing slash
	const wchar_t* p = pathFile;
	wchar_t c;
	while(*p)     ++p;
	while(p > pathFile && (c = *--p) != L'/' && c != L'\\');
	return wstring{ pathFile, p };
}


LinkTargetIDList::LinkTargetIDList(const char* begin, const char* limit) {
	uint16_t listSize = *reinterpret_cast<const uint16_t*>(begin);
	begin += 2;
	if(begin + listSize > limit)   throw errInLNK;
	afterwards = begin + listSize;
	while(*reinterpret_cast<const uint16_t*>(begin)) {
		data.emplace_back(begin, afterwards);
		begin = data.back().dataEnd;
	}
	if((begin += 2) != afterwards)   throw errInLNK;
}


LinkInfo::LinkInfo(const char* begin, const char* limit) {
	const char* begin0 = begin;
	if(begin + 4 > limit)   throw errInLNK;
	auto& p32 = *reinterpret_cast<const uint32_t**>(&begin);
	afterwards = begin + *p32;
	auto nItems = (*++p32 - 12) >> 2; // 4 or 6
	linkInfoFlags = static_cast<LinkInfoFlags>(*++p32);
	const char* itemPointers[6]{ nullptr };
	for(unsigned int i = 0;     i < nItems;     ++i)
		if((itemPointers[i] = begin0 + *++p32) >= afterwards)   throw errInLNK;
	if((int)linkInfoFlags & (int)LinkInfoFlags::VolumeIDAndLocalBasePath) {
		if((begin = itemPointers[0]) > afterwards - 4)   throw errInLNK;
		auto volumeIDsize = *p32;
		if(begin + volumeIDsize > afterwards)   throw errInLNK;
		if(((volumeID = std::make_unique<VolumeIDandBasePath>())->driveType = static_cast<int>(*++p32)) > DRIVE_RAMDISK)
			throw errInLNK;
		volumeID->serialNumber = *++p32;
		if(!(volumeID->volumeLabelIsUnicode = (*++p32 == 0x14))) {
			if((volumeID->volumeLabel = itemPointers[0] + *p32) >= afterwards)   throw errInLNK;
		} else if((volumeID->volumeLabelUC = reinterpret_cast<const wchar_t*>(itemPointers[0] + *++p32)) >= reinterpret_cast<const wchar_t*>(afterwards))
				throw errInLNK;
		volumeID->localBasePath   = itemPointers[1];
		volumeID->localBasePathUC = reinterpret_cast<const wchar_t*>(itemPointers[4]);
	} else p32 += 2;
	//CommonNetworkRelativeLinkOffset <=> itemPointers[2]
	commonPathSuffix = itemPointers[3];
	commonPathSuffixUC = reinterpret_cast<const wchar_t*>(itemPointers[5]);
}



LNK::LNK(const char* begin, const char* end) {
	constexpr static uint32_t header[] = { 76, 0x00021401, 0, 0xC0, 0x46000000 };
	if(end - begin < 76)   throw errInLNK;
	auto& p16 = *reinterpret_cast<const uint16_t**>(&begin);
	auto& p32 = *reinterpret_cast<const uint32_t**>(&begin);
	auto& pwc = *reinterpret_cast<const wchar_t**>(&begin);

	for(int i = 0;     i < 5;     ++i)
		if(*p32++ != header[i])   throw WError(L"Wrong file header — not a proper .lnk file");
	flags = *p32;
	// skipping FileAttributes, CreationTime, AccessTime, WriteTime, FileSize
	iconIdx = *(begin += 36);
	begin += 20; // skipping ShowCommand and HotKey
	if(flags & HasLinkTargetIDList)
		begin = (linkTargetIDList = std::make_unique<LinkTargetIDList>(begin, end))->afterwards;
	if(flags & HasLinkInfo)
		begin = (linkInfo = std::make_unique<LinkInfo>(begin, end))->afterwards;
	const size_t factor = (flags & IsUnicode ? sizeof(wchar_t) : sizeof(char));
	for(auto& p : { std::make_pair(HasName,         &nameString),
	                std::make_pair(HasRelativePath, &relPath),
	                std::make_pair(HasWorkingDir,   &workingDir),
	                std::make_pair(HasArguments,    &commandLine),
	                std::make_pair(HasIconLocation, &iconLoc) })
		if(flags & p.first) {
			auto size = *p16++ * factor;
			(*p.second = std::make_unique<std::string>(begin, begin + size))->push_back('\0');
			begin += size;
			// the extra '\0' above ensures the string is properly terminated even if it is a wide string
		}

}



void print(const wchar_t* s, HANDLE hConsole) {
	DWORD n = 0;
	UINT codepage = GetConsoleOutputCP();
	int bufferSize = WideCharToMultiByte(codepage, 0, s, -1, NULL, 0, NULL, NULL);
	auto buf = std::unique_ptr<char[]>(new char[bufferSize]);
	WideCharToMultiByte(codepage, 0, s, -1, buf.get(), bufferSize, NULL, NULL);
	WriteFile(hConsole, buf.get(), bufferSize - 1, &n, NULL);
}

void print(const char* s, HANDLE hConsole) {
	DWORD n = 0;
	WriteFile(hConsole, s, static_cast<DWORD>(std::strlen(s)), &n, NULL);
}



void outputStringItem(const LNK& lnk, StringItem item, HANDLE hConsole) {
	std::string* s = nullptr;
	switch(item) {
	case StringItem::NAMESTRING:    if(lnk.nameString)    s = lnk.nameString .get();     break;
	case StringItem::RELPATH:       if(lnk.relPath)       s = lnk.relPath    .get();     break;
	case StringItem::WORKINGDIR:    if(lnk.workingDir)    s = lnk.workingDir .get();     break;
	case StringItem::COMMANDLINE:   if(lnk.commandLine)   s = lnk.commandLine.get();     break;
	case StringItem::ICONLOC:       if(lnk.iconLoc)       s = lnk.iconLoc    .get();     break;
	}
	if(!s)   return;
	if(lnk.flags & Flag::IsUnicode)   print(reinterpret_cast<const wchar_t*>(s->c_str()), hConsole);
	else                              print(s->c_str(), hConsole);
	if(item == StringItem::ICONLOC) {
		char buf[32];
		sprintf_s<32>(buf, ",%u", lnk.iconIdx);
		print(buf, hConsole);
	}
}
