; http://nsis.sourceforge.net/Docs/AppendixC.html#C.1
Function GetIEVersion
 Push $R0
   ClearErrors
   ReadRegStr $R0 HKLM "Software\Microsoft\Internet Explorer" "Version"
   IfErrors lbl_123 lbl_456

   lbl_456: ; ie 4+
     Strcpy $R0 $R0 1
   Goto lbl_done

   lbl_123: ; older ie version
     ClearErrors
     ReadRegStr $R0 HKLM "Software\Microsoft\Internet Explorer" "IVer"
     IfErrors lbl_error

       StrCpy $R0 $R0 3
       StrCmp $R0 '100' lbl_ie1
       StrCmp $R0 '101' lbl_ie2
       StrCmp $R0 '102' lbl_ie2

       StrCpy $R0 '3' ; default to ie3 if not 100, 101, or 102.
       Goto lbl_done
         lbl_ie1:
           StrCpy $R0 '1'
         Goto lbl_done
         lbl_ie2:
           StrCpy $R0 '2'
         Goto lbl_done
     lbl_error:
       StrCpy $R0 ''
   lbl_done:
   Exch $R0
 FunctionEnd

; http://nsis.sourceforge.net/VersionCompleteXXXX
!macro VersionCompleteXXXXRevision _INPUT_VALUE _OUTPUT_SYMBOL _REVISION
	!searchparse /noerrors ${_INPUT_VALUE} "" _VERSION_1 "." _VERSION_2 "." _VERSION_3 "." _VERSION_4
	!if `${_VERSION_1}` == ``
		!undef _VERSION_1
		!define _VERSION_1 0
	!endif
	!if `${_VERSION_2}` == ``
		!undef _VERSION_2
		!define _VERSION_2 0
	!endif
	!if `${_VERSION_3}` == ``
		!undef _VERSION_3
		!define _VERSION_3 0
	!endif
	!define ${_OUTPUT_SYMBOL} ${_VERSION_1}.${_VERSION_2}.${_VERSION_3}.${_REVISION}
	!undef _VERSION_1
	!undef _VERSION_2
	!undef _VERSION_3
	!undef _VERSION_4
!macroend
!define VersionCompleteXXXXRevision `!insertmacro VersionCompleteXXXXRevision`
!macro VersionCompleteXXXX _INPUT_VALUE _OUTPUT_SYMBOL
	!searchparse /noerrors ${_INPUT_VALUE} "" _VERSION_1 "." _VERSION_2 "." _VERSION_3 "." _VERSION_4
	!if `${_VERSION_1}` == ``
		!undef _VERSION_1
		!define _VERSION_1 0
	!endif
	!if `${_VERSION_2}` == ``
		!undef _VERSION_2
		!define _VERSION_2 0
	!endif
	!if `${_VERSION_3}` == ``
		!undef _VERSION_3
		!define _VERSION_3 0
	!endif
	!if `${_VERSION_4}` == ``
		!undef _VERSION_4
		!define _VERSION_4 0
	!endif
	!define ${_OUTPUT_SYMBOL} ${_VERSION_1}.${_VERSION_2}.${_VERSION_3}.${_VERSION_4}
	!undef _VERSION_1
	!undef _VERSION_2
	!undef _VERSION_3
	!undef _VERSION_4
!macroend
!define VersionCompleteXXXX `!insertmacro VersionCompleteXXXX`

