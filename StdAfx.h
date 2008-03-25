#define WIN32_LEAN_AND_MEAN
#define STRICT
#pragma warning(disable:4786)

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

#include <EasyReg.h>
#include <StringEx.h>

#undef _FAR_NO_NAMELESS_UNIONS
#include <FAR.h>
#include <FarDlg.h>
#include <FarKeys.hpp>
#include <pcre\pcre.h>
#include <CRegExp.h>
#include <CMapping.h>
#include <Directory.h>
#include <UTF8.h>
#include <Handles.h>
