#define WIN32_LEAN_AND_MEAN
#define STRICT

#define _WIN32_WINNT 0x0500
#define WINVER 0x0500
#include <windows.h>
#include <ole2.h>
#include <comdef.h>
#include <activscp.h>
#include <comcat.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if _MSC_VER == 1400
//#define _Lockit __Lockit
#include <yvals.h>
//#undef _Lockit
#define _Lockit _Lockit_hax
namespace std {
class _Lockit {
public:
	_Lockit() {}
	_Lockit(int) {}
	~_Lockit() {}
};
};
#endif

#include <vector>
#include <string>
#include <map>
#include <deque>
using namespace std;

#undef _FAR_NO_NAMELESS_UNIONS
#include <FAR.h>
#include <FarDlg.h>
#include <FarKeys.hpp>

#include <Pavel.h>
#include <CRegExp.h>
