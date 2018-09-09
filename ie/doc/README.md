# Internet Explorer Development Environment

In order to make changes to, and build the Internet Explorer plugin's dll's i.e: The BHO (Broser Helper Object) you will need to perform the steps below.

## Microsoft Visual Studio 2012
- Install the Professional 2012 version at the minimum, from [here](https://visualstudio.microsoft.com/vs/older-downloads/). Anything newer will require significant project upgrades, and the **Express version will not work since it doesn't come with the ATL libraries**.

## Boost v1.52.0
- Download from [here](https://www.boost.org/users/history/version_1_52_0.html) and copy into `c:\dev\boost_1_52_0`
- Go to `c:\dev\boost_1_52_0`
- Run `bootstrap.bat` then...
- Run `.\b2 --build-type=complete --with-filesystem --with-regex --libdir=c:\dev\boost_1_52_0\stage\lib64 address-model=64`
- Rename `c:\dev\boost_1_52_0\stage\lib` to `c:\dev\boost_1_52_0\stage\lib64`
- Delete `c:\dev\boost_1_52_0\bin.v2`. This is **very important** otherwise you will deal with linking errors for days!
- Run `.\b2 --build-type=complete --with-filesystem --with-regex --libdir=c:\dev\boost_1_52_0\stage\lib address-model=32`

## Opening the Visual Studio Project
- Open [browser-extensions/ie/msvc/BHO.vcxproj](https://github.com/reicolina/browser-extensions/blob/master/ie/msvc/BHO.vcxproj) in Visual Studio 2012.

## Debugging on Windows
- Build solution ForgeIE in debug configuration.
- Install your plugin/extension.
- Replace installed dll's and exe files by ones from Debug directories.
- Also place correspondent pdb files to have ability to interact with source while debugging.
- Modify BHO project setting Debugging->Command to C:\Program Files %28x86%29\Internet Explorer\iexplore.exe
- Select Win32 configuration.
- Press F5.
