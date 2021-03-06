#include "getLNKinfo.h"
#include <fstream>
#include <utility>
#include "resource.h"

using std::string;
using std::wstring;

wstring progFileName;

constexpr COLORREF color1         = RGB(220, 0, 80);
constexpr WORD     color1_console = FOREGROUND_INTENSITY|FOREGROUND_RED;


enum struct Return : int {
	RETURN_OK = 0,
	RETURN_ERROR = 1,
	RETURN_INFO_MISSING = 2
};


enum struct InfoType {
	USE_CONSOLE,
	FILE, PATH, PATHFILE, PATHFILE_REL,
	VOLUME_LABEL, VOLUME_DRIVE_TYPE,
	WORK_DIR, COMMAND_LINE_ARGS,
	ICON, NAME_STRING,
	UNIDENTIFIED, NOFLAG
};


// \x01 switches color to red, \x02 back to default
constexpr const wchar_t* progInfo[]{
	L"Please call the program with the following arguments:",
	L"\x01%s\x02 [\x01/C\x02] [\x01infoType\x02] \x01lnkFilename\x02",
	L"where “\x01lnkFilename\x02” is an absolute or relative link file name (*.lnk),",
	L"optionally “\x01/C\x02” to display error messages in the console instead of msg box",
	L"and “\x01infoType\x02” is an optional flag that specifies what to return. Options are"
};

constexpr std::pair<const wchar_t*, const wchar_t*> infoTypes[]{
	{ L"F",  L"filename the link points to" },
	{ L"P",  L"absolute path of directory the link target is in" },
	{ L"PF", L"absolute path + filename of the link target (default if flag is omitted)" },
	{ L"PR", L"relative path to link target, if it’s stored in the .lnk file" },
	{ L"VL", L"volume label of the drive the link target is in" },
	{ L"VT", L"volume drive type" },
	{ L"W",  L"working directory of the link" },
	{ L"CL", L"command line arguments" },
	{ L"I",  L"link icon" },
	{ L"N",  L"link name string" }
};


InfoType identifyInfoType(const wchar_t* flag) {
	wchar_t c = *flag;
	if(!(c == L'/' || c == L'\\' || (c == '-' && (flag[1] == '-' && ++flag))))   return InfoType::NOFLAG; // "/", "\", "-" or "--"
	std::wstring s{ ++flag };
	for(wchar_t& c : s)     if(c >= L'a' && c <= L'z')   c -= L'a' - L'A'; // make input ASCII uppercase
	if(s.compare(L"C") == 0)   return InfoType::USE_CONSOLE;
	int i = static_cast<int>(InfoType::FILE);
	// since this happens only once per program execution simple linear search is good enough
	for(auto& I : infoTypes)
		if(s.compare(I.first) == 0)   return static_cast<InfoType>(i);
		else ++i;
	return InfoType::UNIDENTIFIED;
}



void printWithHighlights(const wchar_t* str, HDC hDC, LONG left, LONG top) {
	RECT rect{ left, top, left, top }, *pR = &rect;
	const wchar_t* p = str;
	const COLORREF color0 = GetTextColor(hDC);
	while(true)   switch(*p) {
	case L'\0':   case 1:   case 2:
		if(p > str + 1) {
			DrawText(hDC, str, (int)(p - str), pR, DT_CALCRECT);
			DrawText(hDC, str, (int)(p - str), pR, DT_SINGLELINE);
			rect.left = rect.right;
		}
		switch(*p++) {
		case L'\0':   return;
		case 1:       SetTextColor(hDC, color1);   break;
		case 2:       SetTextColor(hDC, color0);   break;
		}
		str = p;
		break;
	default:
		++p;
	}
}

void printWithHighlights(const wchar_t* str, HANDLE hConsole) {
	CONSOLE_SCREEN_BUFFER_INFO buf;
	GetConsoleScreenBufferInfo(hConsole, &buf);
	const wchar_t* p = str;
	DWORD n = 0;
	while(*str) {
		if(*p == 1) {
			while(*++p && *p != 2);
			if(p != ++str) {
				SetConsoleTextAttribute(hConsole, color1_console);
				WriteConsole(hConsole, str, static_cast<DWORD>(p - str), &n, NULL);
				SetConsoleTextAttribute(hConsole, buf.wAttributes);
			}
			str = (*p ? ++p : p);
		} else {
			while(*p && *p != 1)     ++p;
			if(p != str)   WriteConsole(hConsole, str, static_cast<DWORD>(p - str), &n, NULL);
			str = p;
		}
	}
	WriteConsole(hConsole, L"\r\n", 2, &n, NULL);
}



BOOL CALLBACK MsgDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	constexpr static int leftOptionText = 50, margin = 8, indented = 25, lineSkip = 2, lineSkip2 = 5;
	static HFONT hFont = nullptr;
	static LONG width = 500, height = 500, lineH = 15;
	switch(message) {
	case WM_INITDIALOG: {
		RECT rOut, rIn;
		GetClientRect(hwndDlg, &rIn);
		GetWindowRect(hwndDlg, &rOut);
		LONG W = (rOut.right - rOut.left) - (rIn.right - rIn.left),
		     H = (rOut.bottom - rOut.top) - (rIn.bottom - rIn.top);
		SetWindowPos(hwndDlg, HWND_TOPMOST, 0, 0, W + width, H + height, SWP_NOMOVE|SWP_NOZORDER);
		HWND hwndOK = GetDlgItem(hwndDlg, IDOK);
		// now position the OK button on the bottom right corner of the window
		GetWindowRect(hwndOK, &rOut);
		SetWindowPos(hwndOK, HWND_TOPMOST, width  - (rOut.right - rOut.left) - margin,
		                                   height - (rOut.bottom - rOut.top) - margin, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
		break;
	}
	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hwndDlg, &ps);
		HFONT hOldFont;
		LONG y = margin;
		hOldFont = (HFONT)SelectObject(hDC, hFont);
		SetBkMode(hDC, TRANSPARENT);
		wstring buf;
		buf.resize(std::wcslen(progInfo[1]) + progFileName.length());
		wsprintf(&buf.front(), progInfo[1], progFileName.c_str());
		printWithHighlights(progInfo[0], hDC, margin,   y);
		printWithHighlights(buf.c_str(), hDC, indented, y += lineH + lineSkip2);
		printWithHighlights(progInfo[2], hDC, margin,   y += lineH + lineSkip2);
		printWithHighlights(progInfo[3], hDC, margin,   y += lineH + lineSkip);
		printWithHighlights(progInfo[4], hDC, margin,   y += lineH + lineSkip);
		y += lineSkip2 - lineSkip;
		RECT rect{ indented, y, indented, y }, *pR = &rect;
		const COLORREF color0 = SetTextColor(hDC, color1);
		for(const auto& I : infoTypes) {
			rect.bottom += lineH + lineSkip;
			rect.top    += lineH + lineSkip;
			DrawText(hDC, ((buf = L'/') += I.first).c_str(), -1, pR, DT_SINGLELINE | DT_NOCLIP);
		}
		SetTextColor(hDC, color0);
		rect = RECT{ leftOptionText, y, leftOptionText, y + lineH + lineSkip };
		for(const auto& I : infoTypes) {
			rect.bottom += lineH + lineSkip;
			rect.top    += lineH + lineSkip;
			DrawText(hDC, I.second, -1, pR, DT_SINGLELINE|DT_NOCLIP);
		}
		SelectObject(hDC, hOldFont);
		EndPaint(hwndDlg, &ps);
		break;
	}
	case WM_SETFONT: // With modal dialogs this event gets called before WM_INITDIALOG, which is a blessing
		hFont = (HFONT)wParam;
		{
			size_t max = 0, idxMax = 0;
			for(size_t i = 0;     i < std::size(infoTypes);     ++i)
				if(std::wcslen(infoTypes[i].second) > max)   { max = std::wcslen(infoTypes[i].second);     idxMax = i; }
			RECT rect{ leftOptionText, 0, leftOptionText, 0 };
			HDC hDC = GetDC(hwndDlg);
			SelectObject(hDC, hFont);
			DrawText(hDC, infoTypes[idxMax].second, static_cast<int>(max), &rect, DT_CALCRECT);
			ReleaseDC(hwndDlg, hDC);
			lineH  = rect.bottom;
			width  = rect.right + margin;
			height = static_cast<LONG>(3 * margin + 3 * lineSkip2 - 2 * lineSkip + static_cast<LONG>(std::size(infoTypes) + 5) * (lineH + lineSkip));
		}
		break;
	case WM_COMMAND:
		EndDialog(hwndDlg, wParam);
		return TRUE;
	}
	return FALSE;
}



int printDescription(bool useConsole) {
	if(!useConsole) {
		DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), GetConsoleWindow(), (DLGPROC)MsgDialogProc);
		return EXIT_FAILURE;
	}
	constexpr static size_t indent = 2;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	wstring buf;
	buf.resize(std::wcslen(progInfo[1]) + indent + progFileName.length(), L' ');
	wsprintf(&buf[indent], progInfo[1], progFileName.c_str());
	printWithHighlights(progInfo[0], hConsole);
	printWithHighlights(buf.c_str(), hConsole);
	printWithHighlights(progInfo[2], hConsole);
	printWithHighlights(progInfo[3], hConsole);
	printWithHighlights(progInfo[4], hConsole);
	for(const auto& P : infoTypes) {
		buf.clear();     buf.resize(indent, L' ');
		(((buf += L"\x01/") += P.first) += L'\x02').resize(9, L' ');
		printWithHighlights((buf += P.second).c_str(), hConsole);
	}
	return EXIT_FAILURE;
}


wstring& operator+=(wstring& s, const char* cs) {
	while(*cs)   s += static_cast<wchar_t>(*cs++);
	return s;
}


/* The main program */
int wmain(int argc, wchar_t* const argv[]) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	UINT codepage = GetConsoleOutputCP();
	auto print = [=](const wchar_t* s) {
		DWORD n = 0;
		int bufferSize = WideCharToMultiByte(codepage, 0, s, -1, NULL, 0, NULL, NULL);
		auto buf = std::unique_ptr<char[]>(new char[bufferSize]);
		WideCharToMultiByte(codepage, 0, s, -1, buf.get(), bufferSize, NULL, NULL);
		WriteFile(hConsole, buf.get(), bufferSize - 1, &n, NULL);
	};
	auto print2 = [=](const char* s) {
		DWORD n = 0;
		WriteFile(hConsole, s, static_cast<DWORD>(std::strlen(s)), &n, NULL);
	};
	bool useConsole = false;
	if(argc <= 0)   return EXIT_FAILURE;
	wchar_t* const* arg = argv;
	progFileName = takeFilename(*arg++);
	InfoType type = InfoType::PATHFILE;
	if(--argc == 0)   return printDescription(useConsole); // no explicit arguments given
	const wchar_t* pathFile = nullptr;
	if((type = identifyInfoType(pathFile = *arg++)) == InfoType::USE_CONSOLE) {
		useConsole = true;
		if(--argc == 0)   return printDescription(useConsole); // "getLNKinfo.exe /C"
		type = identifyInfoType(pathFile = *arg++);
	}
	if(type == InfoType::UNIDENTIFIED || type == InfoType::USE_CONSOLE)   return printDescription(useConsole);
	if(type != InfoType::NOFLAG) {
		if(--argc == 0)   return printDescription(useConsole); // "getLNKinfo.exe [/C] /FLAG"
		if(identifyInfoType(pathFile = *arg++) != InfoType::NOFLAG)
			return printDescription(useConsole); // "getLNKinfo.exe [/C] /FLAG /FLAG2"
	} else type = InfoType::PATHFILE; // default info to return
	try {
		std::ifstream file{ pathFile, std::ios_base::binary|std::ios_base::in };
		if(!file.is_open())   throw WError(L"Link file doesn’t exist or could not be opened!");
		std::string contentStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
		const char* content = contentStr.data(), * content_e = content + contentStr.size();
		LNK lnk(content, content_e);
		switch(type) {
		case InfoType::FILE:
		case InfoType::PATH:
		case InfoType::PATHFILE:
			if(!lnk.linkInfo)   return EXIT_SUCCESS;
			{
				wstring s;
				if(lnk.linkInfo->volumeID)
					if(lnk.linkInfo->volumeID->localBasePathUC)   s += lnk.linkInfo->volumeID->localBasePathUC;
					else                                          s += lnk.linkInfo->volumeID->localBasePath;
				if(lnk.linkInfo->commonPathSuffixUC)   s += lnk.linkInfo->commonPathSuffixUC;
				else                                   s += lnk.linkInfo->commonPathSuffix;
				if(type == InfoType::PATHFILE)    print(s.c_str());
				else if(type == InfoType::FILE)   print(takeFilename(s.c_str()).c_str());
				else                              print(takePathname(s.c_str()).c_str());
			}
			break;
		case InfoType::VOLUME_DRIVE_TYPE:
			if(!lnk.linkInfo || !lnk.linkInfo->volumeID)   return EXIT_SUCCESS;
			print2(driveTypeNames[lnk.linkInfo->volumeID->driveType]);
			break;
		case InfoType::VOLUME_LABEL:
			if(!lnk.linkInfo || !lnk.linkInfo->volumeID)   return EXIT_SUCCESS;
			if(lnk.linkInfo->volumeID->volumeLabelIsUnicode)   print (lnk.linkInfo->volumeID->volumeLabelUC);
			else                                               print2(lnk.linkInfo->volumeID->volumeLabel);
			break;
		case InfoType::WORK_DIR:
			outputStringItem(lnk, StringItem::WORKINGDIR, hConsole);
			break;
		case InfoType::ICON:
			outputStringItem(lnk, StringItem::ICONLOC, hConsole);
			break;
		case InfoType::PATHFILE_REL:
			outputStringItem(lnk, StringItem::RELPATH, hConsole);
			break;
		case InfoType::NAME_STRING:
			outputStringItem(lnk, StringItem::NAMESTRING, hConsole);
			break;
		case InfoType::COMMAND_LINE_ARGS:
			outputStringItem(lnk, StringItem::COMMANDLINE, hConsole);
			break;
		}
		print(L"\r\n");
		return EXIT_SUCCESS;
	}
	catch(const WError & err) {
		auto title = wstring{ reinterpret_cast<const wchar_t*>(u"Error reading link file “") } + pathFile + L'”';
		if(useConsole)   print((((title += L"\r\n") += err.wwhat()) += L"\r\n").c_str());
		else             MessageBox(NULL, err.wwhat(), title.c_str(), MB_OK|MB_ICONWARNING);
		return 2;
	}
	return EXIT_FAILURE;
}
