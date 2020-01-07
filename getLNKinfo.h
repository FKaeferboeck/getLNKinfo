#pragma once
#include <string>
#include <stdexcept>
#include <vector>
#include <stdexcept>
#include <memory>
#include <type_traits>
#include <Windows.h>


struct LNK_error : std::runtime_error {
	int arg1, arg2;
	LNK_error(int arg1, int arg2, const char* msg) : arg1(arg1), arg2(arg2), std::runtime_error(msg) { }
};


class WError : public std::runtime_error {
	static std::string convert(const std::wstring& s)
		{ return std::string(reinterpret_cast<const char*>(&s.front()), 2 * s.length()); }
	std::wstring str;
public:
	WError(const std::wstring& s) : std::runtime_error(convert(s)), str(s) { }
	const wchar_t* wwhat() const { return /*reinterpret_cast<const wchar_t*>(what());*/ str.c_str(); }
};


std::wstring takeFilename(const wchar_t* pathFile);
std::wstring takePathname(const wchar_t* pathFile); // without trailing slash


enum Flag : uint32_t {
	HasLinkTargetIDList         = 1,
	HasLinkInfo                 = 1 <<  1,
	HasName                     = 1 <<  2,
	HasRelativePath             = 1 <<  3,
	HasWorkingDir               = 1 <<  4,
	HasArguments                = 1 <<  5,
	HasIconLocation             = 1 <<  6,
	IsUnicode                   = 1 <<  7,
	ForceNoLinkInfo             = 1 <<  8,
	HasExpString                = 1 <<  9,
	RunInSeparateProces         = 1 << 10,
	HasDarwinID                 = 1 << 12,
	RunAsUser                   = 1 << 13,
	HasExpIcon                  = 1 << 14,
	NoPidlAlias                 = 1 << 15,
	RunWithShimLayer            = 1 << 17,
	ForceNoLinkTrack            = 1 << 18,
	EnableTargetMetadata        = 1 << 19,
	DisableLinkPathTracking     = 1 << 20,
	DisableKnownFolderTracking  = 1 << 21,
	DisableKnownFolderAlias     = 1 << 22,
	AllowLinkToLink             = 1 << 23,
	UnaliasOnSave               = 1 << 24,
	PreferEnvironmentPath       = 1 << 25,
	KeepLocalIDListForUNCTarget = 1 << 26
};


constexpr const char* driveTypeNames[]
	{ "DRIVE_UNKNOWN", "DRIVE_NO_ROOT_DIR", "DRIVE_REMOVABLE", "DRIVE_FIXED", "DRIVE_REMOTE", "DRIVE_CDROM", "DRIVE_RAMDISK" };


struct VolumeIDandBasePath {
	int driveType; // as defined in WinBase.h
	uint32_t  serialNumber;
	bool volumeLabelIsUnicode;
	union {
		const char*    volumeLabel;
		const wchar_t* volumeLabelUC;
	};
	const char*    localBasePath;
	const wchar_t* localBasePathUC;
};


enum struct LinkInfoFlags : uint32_t {
	VolumeIDAndLocalBasePath = 1,
	CommonNetworkRelativeLinkAndPathSuffix = 2
};


struct ItemID {
	const char *dataBegin, *dataEnd;
	ItemID(const char *begin, const char* limit) : dataBegin(begin + 2), dataEnd(begin + *reinterpret_cast<const uint16_t*>(begin))
		{ if(dataEnd > limit)   throw std::runtime_error{ "Error" }; }
	ItemID() = default;
	ItemID(const ItemID&) = default;
};


struct LinkTargetIDList {
	std::vector<ItemID> data;
	const char* afterwards;
	LinkTargetIDList(const char* begin, const char* limit);
};


struct LinkInfo {
	LinkInfoFlags linkInfoFlags;
	std::unique_ptr<VolumeIDandBasePath> volumeID;
	const char*    commonPathSuffix;
	const wchar_t* commonPathSuffixUC;
	const char* afterwards;
	LinkInfo(const char* begin, const char* limit);
};



struct LNK {
	uint32_t flags;
	std::unique_ptr<LinkTargetIDList> linkTargetIDList;
	std::unique_ptr<LinkInfo>         linkInfo;
	std::unique_ptr<std::string> nameString, relPath, workingDir, commandLine, iconLoc;
	uint32_t iconIdx;
	LNK(const char* begin, const char* end);
};


enum struct StringItem { NAMESTRING, RELPATH, WORKINGDIR, COMMANDLINE, ICONLOC };

void outputStringItem(const LNK& lnk, StringItem item, HANDLE hConsole);
