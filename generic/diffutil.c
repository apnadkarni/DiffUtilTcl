#include <tcl.h>

extern int DiffFilesObjCmd(
    ClientData dummy,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]);	

#if defined(_MSC_VER)
#   define EXPORT(a,b) __declspec(dllexport) a b
#   define DllEntryPoint DllMain
#else
#   if defined(__BORLANDC__)
#       define EXPORT(a,b) a _export b
#   else
#       define EXPORT(a,b) a b
#   endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
EXPORT(int,Diffutil_Init) (Tcl_Interp *interp);
EXPORT(int,Diffutil_SafeInit) (Tcl_Interp *interp);
#ifdef __cplusplus
}
#endif

#if defined(__WIN32__)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN

BOOL WINAPI
DllEntryPoint(HINSTANCE hinstDll, DWORD fdwReason, LPVOID lpvReserved)
{
    return 1;
}

#endif /* __WIN32__ */

/* 'string first' for unicode string */
static int
UniCharFirst(ustring1, length1, ustring2, length2)
    Tcl_UniChar *ustring1;
    int length1;
    Tcl_UniChar *ustring2;
    int length2;
{
    int match, start = 0;

    /*
     * We are searching string2 for the sequence string1.
     */
    
    match = -1;
    start = 0;
    if (length1 < 0)
	length1 = Tcl_UniCharLen(ustring1);
    if (length2 < 0)
	length2 = Tcl_UniCharLen(ustring2);

    if (start != 0) {
	if (start >= length2) {
	    return -1;
	} else if (start > 0) {
	    ustring2 += start;
	    length2  -= start;
	} else if (start < 0) {
	    start = 0;
	}
    }

    if (length1 > 0) {
	register Tcl_UniChar *p, *end;

	end = ustring2 + length2 - length1 + 1;
	for (p = ustring2;  p < end;  p++) {
	    /*
	     * Scan forward to find the first character.
	     */
	    if ((*p == *ustring1) &&
		    (Tcl_UniCharNcmp(ustring1, p,
			    (unsigned long) length1) == 0)) {
		match = p - ustring2;
		break;
	    }
	}
    }
    /*
     * Compute the character index of the matching string by
     * counting the number of characters before the match.
     */
    if ((match != -1) && (start != 0)) {
	match += start;
    }

    return match;
}

// Recursively look for common substrings in strings str1 and str2
// res1 and res2 should point to list objects where the result
// will be appended.
static void
CompareMidString(interp, obj1, obj2, res1, res2, wordparse)
    Tcl_Interp *interp;
    Tcl_Obj *obj1, *obj2;
    Tcl_Obj *res1, *res2;
    int wordparse;
{
    Tcl_UniChar *str1, *str2;
    int len1, len2, t, u, i, newt, newp1;
    int p1, p2, found1 = 0, found2 = 0, foundlen, minlen;
    Tcl_Obj *apa1, *apa2;

    str1 = Tcl_GetUnicodeFromObj(obj1, &len1);
    str2 = Tcl_GetUnicodeFromObj(obj2, &len2);

    // Is str1 a substring of str2 ?
    if (len1 < len2) {
	if ((t = UniCharFirst(str1, len1, str2, len2)) != -1) {
	    Tcl_ListObjAppendElement(interp, res2,
		    Tcl_NewUnicodeObj(str2, t));
	    Tcl_ListObjAppendElement(interp, res2,
		    Tcl_NewUnicodeObj(str2 + t, len1));
	    Tcl_ListObjAppendElement(interp, res2,
		    Tcl_NewUnicodeObj(str2 + t + len1, -1));
	    Tcl_ListObjAppendElement(interp, res1, Tcl_NewObj());
	    Tcl_ListObjAppendElement(interp, res1, obj1);
	    Tcl_ListObjAppendElement(interp, res1, Tcl_NewObj());
	    return;
	}
    }

    // Is str2 a substring of str1 ?
    if (len2 < len1) {
	if ((t = UniCharFirst(str2, len2, str1, len1)) != -1) {
	    Tcl_ListObjAppendElement(interp, res1,
		    Tcl_NewUnicodeObj(str1, t));
	    Tcl_ListObjAppendElement(interp, res1,
		    Tcl_NewUnicodeObj(str1 + t, len2));
	    Tcl_ListObjAppendElement(interp, res1,
		    Tcl_NewUnicodeObj(str1 + t + len2, -1));
	    Tcl_ListObjAppendElement(interp, res2, Tcl_NewObj());
	    Tcl_ListObjAppendElement(interp, res2, obj2);
	    Tcl_ListObjAppendElement(interp, res2, Tcl_NewObj());
	    return;
	}
    }

    // Are they too short to be considered ?
    if (len1 < 4 || len2 < 4) {
	Tcl_ListObjAppendElement(interp, res1, obj1);
	Tcl_ListObjAppendElement(interp, res2, obj2);
        return;
    }

    // Find the longest string common to both strings

    foundlen = -1;
    minlen = 2; // The shortest common substring we detect is 3 chars

    for (t = 0, u = minlen; u < len1; t++, u++) {
        i = UniCharFirst(str1 + t, u - t + 1, str2, len2);
        if (i >= 0) {
            for (p1 = u + 1, p2 = i + minlen + 1; p1 < len1 && p2 < len2;
		 p1++, p2++) {
                if (str1[p1] != str2[p2]) break;
	    }
            if (wordparse) {
                newt = t;
                if ((t > 0 && Tcl_UniCharIsWordChar(str1[t-1])) ||
			(i > 0 && Tcl_UniCharIsWordChar(str2[i-1]))) {
                    for (; newt < p1; newt++) {
                        if (!Tcl_UniCharIsWordChar(str1[newt])) break;
                    }
                }

                newp1 = p1 - 1;
                if ((p1 < len1 && Tcl_UniCharIsWordChar(str1[p1])) ||
			(p2 < len2 && Tcl_UniCharIsWordChar(str2[p2]))) {
                    for (; newp1 > newt; newp1--) {
                        if (!Tcl_UniCharIsWordChar(str1[newp1])) break;
                    }
                }
                newp1++;

                if (newp1 - newt > minlen) {
                    foundlen = newp1 - newt;
                    found1 = newt;
                    found2 = i + newt - t;
                    minlen = foundlen;
                    u = t + minlen;
                }
            } else {
                foundlen = p1 - t;
                found1 = t;
                found2 = i;
                minlen = foundlen;
                u = t + minlen;
            }
        }
    }

    if (foundlen < 0) {
	// No common string found
	Tcl_ListObjAppendElement(interp, res1, obj1);
	Tcl_ListObjAppendElement(interp, res2, obj2);
        return;
    }

    // Handle left part, recursively
    apa1 = Tcl_NewUnicodeObj(str1, found1);
    apa2 = Tcl_NewUnicodeObj(str2, found2);
    Tcl_IncrRefCount(apa1);
    Tcl_IncrRefCount(apa2);
    CompareMidString(interp, apa1, apa2, res1, res2, wordparse);
    Tcl_DecrRefCount(apa1);
    Tcl_DecrRefCount(apa2);

    // Handle middle (common) part
    Tcl_ListObjAppendElement(interp, res1,
	    Tcl_NewUnicodeObj(str1 + found1, foundlen));
    Tcl_ListObjAppendElement(interp, res2,
	    Tcl_NewUnicodeObj(str2 + found2, foundlen));
    
    // Handle right part, recursively
    apa1 = Tcl_NewUnicodeObj(str1 + found1 + foundlen, -1);
    apa2 = Tcl_NewUnicodeObj(str2 + found2 + foundlen, -1);
    Tcl_IncrRefCount(apa1);
    Tcl_IncrRefCount(apa2);
    CompareMidString(interp, apa1, apa2, res1, res2, wordparse);
    Tcl_DecrRefCount(apa1);
    Tcl_DecrRefCount(apa2);

}

int
CompareLinesObjCmd(dummy, interp, objc, objv)
    ClientData dummy;    	/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    int index, t, result = TCL_OK;
    int ignore = 0, wordparse = 0;
    int len1, len2;
    Tcl_UniChar *line1, *line2, *s1, *s2, *e1, *e2;
    //char *line1, *line2, *s1, *s2, *e1, *e2, *prev, *prev2;
    Tcl_UniChar *word1, *word2;
    int wordflag;
    Tcl_Obj *res1, *res2, *mid1, *mid2;
    static CONST char *options[] = {
	"-b", "-w", "-word", (char *) NULL
    };
    enum options {
	OPT_B, OPT_W, OPT_WORD
    };	  

    if (objc < 5) {
        Tcl_WrongNumArgs(interp, 1, objv, "?opts? line1 line2 res1Name res2Name");
	return TCL_ERROR;
    }
    
    for (t = 1; t < objc - 4; t++) {
	if (Tcl_GetIndexFromObj(interp, objv[t], options, "option", 0,
		&index) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch (index) {
	  case OPT_B:
	    ignore = 1;
	    break;
	  case OPT_W:
	    ignore = 2;
	    break;
	  case OPT_WORD:
	    wordparse = 1;
	    break;
	}
    }

    line1 = Tcl_GetUnicodeFromObj(objv[objc-4], &len1);
    line2 = Tcl_GetUnicodeFromObj(objv[objc-3], &len2);
    
    s1 = line1;
    s2 = line2;
    e1 = line1 + len1;
    e2 = line2 + len2;

    // Skip whitespace in both ends
    if (ignore > 0) {
	while (s1 < e1 && Tcl_UniCharIsSpace(*s1)) s1++;
	while (s2 < e2 && Tcl_UniCharIsSpace(*s2)) s2++;
	while (e1 > s1 && Tcl_UniCharIsSpace(*(e1-1))) e1--;
	while (e2 > s2 && Tcl_UniCharIsSpace(*(e2-1))) e2--;
    }
    
    // Skip matching chars in both ends
    // Forwards
    word1 = s1; word2 = s2;
    wordflag = 0;
    while (s1 < e1 && s2 < e2) {
	if (wordflag) {
	    word1 = s1;
	    word2 = s2;
	}
	if (*s1 != *s2) break;
	if (wordparse) {
	    if (Tcl_UniCharIsWordChar(*s1)) {
		wordflag = 0;
	    } else {
		wordflag = 1;
		word1 = s1;
		word2 = s2;
	    }
	}
	s1++;
	s2++;
    }
    if (wordparse) {
	s1 = word1;
	s2 = word2;
    }
    // Backwards
    word1 = e1; word2 = e2;
    wordflag = 0;
    while (e1 > s1 && e2 > s2) {
	if (wordflag) {
	    word1 = e1;
	    word2 = e2;
	}
	if (*(e1 - 1) != *(e2 - 1)) break;
	if (wordparse) {
	    if (Tcl_UniCharIsWordChar(*(e1 - 1))) {
		wordflag = 0;
	    } else {
		wordflag = 1;
		word1 = e1;
		word2 = e2;
	    }
	}
	e1--;
	e2--;
    }
    if (wordparse) {
	e1 = word1;
	e2 = word2;
    }

    res1 = Tcl_NewListObj(0, NULL);
    Tcl_IncrRefCount(res1);
    Tcl_ListObjAppendElement(interp, res1, Tcl_NewUnicodeObj(line1, s1-line1));
    res2 = Tcl_NewListObj(0, NULL);
    Tcl_IncrRefCount(res2);
    Tcl_ListObjAppendElement(interp, res2, Tcl_NewUnicodeObj(line2, s2-line2));

    if (e1 > s1 || e2 > s2) {
	mid1 = Tcl_NewUnicodeObj(s1, e1 - s1);
	mid2 = Tcl_NewUnicodeObj(s2, e2 - s2);
	Tcl_IncrRefCount(mid1);
	Tcl_IncrRefCount(mid2);
	CompareMidString(interp, mid1, mid2, res1, res2, wordparse);
	Tcl_DecrRefCount(mid1);
	Tcl_DecrRefCount(mid2);

	Tcl_ListObjAppendElement(interp, res1, Tcl_NewUnicodeObj(e1, -1));
	Tcl_ListObjAppendElement(interp, res2, Tcl_NewUnicodeObj(e2, -1));
    }

    if (Tcl_ObjSetVar2(interp, objv[objc-2], NULL, res1, TCL_LEAVE_ERR_MSG)
	    == NULL) {
	result = TCL_ERROR;
	goto endCompareLines;
    }

    if (Tcl_ObjSetVar2(interp, objv[objc-1], NULL, res2, TCL_LEAVE_ERR_MSG)
	    == NULL) {
	result = TCL_ERROR;
	goto endCompareLines;
    }

    endCompareLines:
    Tcl_DecrRefCount(res1);
    Tcl_DecrRefCount(res2);
    return result;
}

#define TCC(name,func) \
Tcl_CreateCommand(interp, name, func, (ClientData) NULL, NULL)
#define TCOC(name,func) \
Tcl_CreateObjCommand(interp, name, func, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL)

EXPORT(int,Diffutil_Init) (Tcl_Interp *interp)
{
    if (Tcl_InitStubs(interp, "8.2", 0) == NULL) {
	return TCL_ERROR;
    }

    if (Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION) != TCL_OK) {
	return TCL_ERROR;
    }

    TCOC("DiffUtil::diffFiles", DiffFilesObjCmd);
    TCOC("DiffUtil::compareLines", CompareLinesObjCmd);
    Tcl_SetVar(interp, "DiffUtil::version", PACKAGE_VERSION, TCL_GLOBAL_ONLY);

    return TCL_OK;
}

EXPORT(int,Diffutil_SafeInit) (Tcl_Interp *interp)
{
    return TCL_OK;
}
