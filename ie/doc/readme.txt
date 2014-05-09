-*- mode: org; -*-

* development environment
** NSIS 2.46
   install 
** MSVC 2012
   install
** boost-1.52.0
   copy into c:\dev\boost_1_52_0
   bootstrap.bat
   .\b2 --build-type=complete --with-filesystem --with-regex --libdir=c:\dev\boost_1_52_0\stage\lib64 address-model=64
   # Rename c:\dev\boost_1_52_0\stage\lib to c:\dev\boost_1_52_0\stage\lib64
   .\b2 --build-type=complete --with-filesystem --with-regex --libdir=c:\dev\boost_1_52_0\stage\lib address-model=32

* building
** run in root directory:
   build.bat debug test-platform testplatform
   build.bat release test-platform testplatform

* running under a user account on vmware
** Map shares to "Run As Administrator" command prompt:
   caspol -m -ag 1.5 -url file://z:/* FullTrust
   net use z: "\\vmware-host\Shared Folders"
OR
   net use z: \\192.168.20.105\trigger * /user:antoine

* signing
** read
   http://www.top20toolbar.com/misc/codesigncert.htm
   http://www.matthew-jones.com/articles/codesigning.html
** generate root ca certificate for testing
   openssl genrsa -des3 -out ca.key 4096
   openssl req -new -x509 -days 365 -key ca.key -out ca.crt
** generate a customer certificate
   openssl genrsa -des3 -out customer.key 4096
   openssl req -new -key customer.key -out customer.csr  # skip last 2 optional attributes
   openssl x509 -req -days 365 -in customer.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out customer.crt
   openssl pkcs12 -export -out windows.pfx -inkey customer.key -in customer.crt
** sign BHO
   signtool sign /f dist\certs.dev\exported.pfx /p "somepassword" 
                 /v /t http://timestamp.comodoca.com/authenticode 
                 build\x64\Debug\bho64.dll
