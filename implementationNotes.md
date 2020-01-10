# Implementation notes for *getLNKinfo.exe*

## The ugly truth about the Windows console
Windows natively has the ability to work with Unicode in the form of UTF-16 (wide character strings).

However the Windows console for some reason does not; it only works with codepages, which can also be multi-byte types such as UTF-8. To display non-ASCII
characters in the console you first set the console to a codepage that contains those characters, convert the string from `wchar_t*` (UTF-16) to that codepage
(which uses `char*`), then display it in a number of ways (`std::cout`, `printf`, `WriteFile` etc.).

The Windows API provides the function `WriteConsoleW`, which takes a wide string and performs those steps implicitly.


But there is a problem:
*getLNKinfo.exe* will often be used with **redirected output**, for example in a batch file to store its output into a variable:

    FOR /F "tokens=*" %%i IN ('getLNKinfo.exe /F exampleLinkFile.lnk') DO SET var=%%i

(This command temporarily redirects its output away from the console.)

Since *getLNKinfo.exe* gets called on an already redirected console handle and codepages are something that only applies to consoles, not other output objects
like files, the program cannot change the codepage to display special characters. Likewise `WriteConsoleW` is [not available](https://docs.microsoft.com/en-us/windows/console/high-level-console-input-and-output-functions) for redirected console handles.

Therefore we need to perform the steps described above manually, as the following example shows:
```c++
const wchar_t* example = L"Lemmy “Motörhead” Kilmister";

UINT codepage = GetConsoleOutputCP();
int bufferSize = WideCharToMultiByte(codepage, 0, example, -1, NULL, 0, NULL, NULL); // measure string length after conversion
auto buf = std::unique_ptr<char[]>(new char[bufferSize]);
WideCharToMultiByte(codepage, 0, example, -1, buf.get(), bufferSize, NULL, NULL); // convert string to current console codepage

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
DWORD n = 0;
WriteFile(hConsole, buf.get(), bufferSize - 1, &n, NULL); // write converted string to console, excluding terminating '\0'
```
The snippet above will display `Lemmy “Motörhead” Kilmister` when called with a codepage active that can display it (e.g. CP 65001 = UTF-8), and `Lemmy "Motorhead" Kilmister` otherwise.

## Modal dialog shenanigans
If called with wrong arguments *getLNKinfo.exe* will display “how to use” info either in the console or as a dialog box. For this we use a Windows **modal dialog**. It would be of course a lot easier to use a simple message box instead of an elaborate cusotm dialog, but I wanted to highlight parts of the text in color, which a message box can't.

A modal dialog is created from a template stored in the project’s resource file (*getLNKinfo.rc*). Normally this template contains the text pieces to display in the dialog as *static controls*. When rendering the dialog for each static control a `WM_CTLCOLORSTATIC` event is received which can be used to set the color for the highlighted items (the colors are not contained in the dialog template). However, unsurprisingly, I chose a more complicated way once again.

That's because I want to set the dialog text dynamically. In particular it uses the file name of the program itself, which may have been changed. The content of a static control is easy to change, but the following content needs to be horizontally aligned to its size, so I decided to leave the dialog window empty except for the OK button and write its content manually in response to the `WM_PAINT` event.

This is done by the `DrawText` function (actually `DrawTextW`), as usual between `BeginPaint` and `EndPaint`, but there is the question of the correct **text font**, which is annoying:

* It's not one of the Windows stock fonts (e.g. `DEFAULT_GUI_FONT` or `SYSTEM_FONT`), because those [are not in use any more](https://devblogs.microsoft.com/oldnewthing/20050707-00/?p=35013).
* Neither is it one of the system fonts from the `NONCLIENTMETRICS` structure:
```c++
NONCLIENTMETRICS metr{ sizeof(NONCLIENTMETRICS) };
SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metr, 0);
HFONT hFont = CreateFontIndirect(&metr.lfMessageFont); // Wrong font!
// and neither is it metr.lfCaptionFont, metr.lfMenuFont, metr.lfSmCaptionFont, or metr.lfStatusFont
```

It is a different font altogether. If anybody knows why, please tell me!

Thankfully we can at least access that font, because modal dialogs send a `WM_SETFONT` message before `WM_INITDIALOG`, which contains the font handle in its `wParam` value.

The rest is straightforward: We call `DrawText` twice for each text item, first time with flag `DT_CALCRECT` to calculate the pixel size of the text; we set the window size with `SetWindowPos` (and also use it to position the OK button), adding the size difference between the outer window size and its client area by comparing `GetWindowRect` and `GetClientRect`.
