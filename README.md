# getLNKinfo
Command line utility for extracting information from **Microsoft Shell Link** (*.lnk*) files

This project is a small *.exe* utility for Windows to extract various information contained in Microsoft's Shell Link (*.lnk*) format.

The project is built with Microsoft Visual Studio 64 bit via CMake and uses C++14.

During the development of this program it became apparent that I had no use for it after all; so continued development is not to be expected. In particular
*.lnk* files can contain a number of optional data items that are not implemented in *getLNKinfo.exe*. You may add them yourself at your own leisure.

The full documentation of the Microsoft Shell Link format can be found here:
[MS-SHLLINK](https://docs.microsoft.com/de-de/openspecs/windows_protocols/ms-shllink)

## How to use the compiled program:
Please call the program with the following arguments:
> **getLNKinfo.exe** [**/C**] [**infoType**] **lnkFilename**

where “**lnkFilename**” is an absolute or relative link file name (*.lnk),
optionally “**/C**” to display error messages in the console instead of msg box
and “**infoType**” is an optional flag that specifies what to return. Options are
- **/F**   filename the link points to
- **/P**   absolute path of directory the link target is in
- **/PF**  absolute path + filename of the link target (default if flag is omitted)
- **/PR**  relative path to link target, if it’s stored in the .lnk file
- **/VL**  volume label of the drive the link target is in
- **/VT**  volume drive type
- **/W**   working directory of the link
- **/CL**  command line arguments
- **/I**   link icon
- **/N**   link name string
