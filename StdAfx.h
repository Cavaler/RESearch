#define WIN32_LEAN_AND_MEAN
#define STRICT

#define _WIN32_WINNT 0x0501
#ifndef _WIN64
#define _USE_32BIT_TIME_T
#endif

#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <comdef.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if _MSC_VER >= 1400
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
#include <set>
#include <deque>
#include <algorithm>
#include <memory>
using namespace std;

#include <Pavel.h>
#include <CRegExp.h>
#include <DebugTimer.h>

#import "RESearch.tlb" raw_interfaces_only
using namespace RESearchLib;
