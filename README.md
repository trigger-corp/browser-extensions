OpenForge
=========

Build and run cross-platform browser extensions from one codebase.

Please see the [introductory blog post][intro-blog] for details on the
history of this project.

Documentation [can be found here][docs].


Dependencies
------------


### Ubuntu

* Python dependencies: `sudo apt-get install python-setuptools python-pip python-virtualenv`
* Mercurial: `sudo apt-get install mercurial`
* Git: `sudo apt-get install git`
* Java Runtime: `sudo apt-get install default-jre`
* NSIS: `sudo apt-get install nsis` (Only needed if you want to do IE builds)

### OSX:

* Python dependencies: `sudo easy_install pip virtualenv`
* Mercurial: [http://mercurial.selenic.com](http://mercurial.selenic.com)
* Git: [http://git-scm.com](http://git-scm.com)

### Windows::

* Python 2.7.5: [http://www.python.org/ftp/python/2.7.5/python-2.7.5.msi](http://www.python.org/ftp/python/2.7.5/python-2.7.5.msi)

  Add the following paths: (Control Panel -> System -> Advanced -> Environment Variables)

      C:\Python27;C:\Python27\Scripts

* Python setuptools: [https://pypi.python.org/pypi/setuptools/1.0#windows](https://pypi.python.org/pypi/setuptools/1.0#windows)
* Python dependencies: `easy_install pip virtualenv`
* Mercurial: [http://mercurial.selenic.com](http://mercurial.selenic.com)
* Git: [http://git-scm.com/download/win](http://git-scm.com/download/win) (Choose "Run Git from the Windows Command Prompt option!)
* Putty: [http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html](http://www.chiark.greenend.org.uk/~sgtatham/putty/download.html)
* Java Runtime: [http://java.com/en/download/index.jsp](http://java.com/en/download/index.jsp)
* NSIS: [http://nsis.sourceforge.net/](http://nsis.sourceforge.net/) (Version 2.46)

  Add the following paths: (Control Panel -> System -> Advanced -> Environment Variables)

	  C:\Program Files (x86)\NSIS

* signtool: [http://msdn.microsoft.com/en-us/library/windows/desktop/aa387764(v=vs.85).aspx](http://msdn.microsoft.com/en-us/library/windows/desktop/aa387764\(v=vs.85\).aspx)



Installation
------------

### Clone Repo:

    git clone https://github.com/trigger-corp/browser-extensions.git

### Create Python Environment:

    cd browser-extensions
    virtualenv --no-site-packages ./python-env

### Activate Python Environment (Unix):

    source ./python-env/bin/activate

### Activate Python Environment (Windows):

    .\python-env\Scripts\activate.bat

### Setup Python Environment:

    cd generate
    pip install -r requirements.txt
    python setup.py develop
    cd ..



Using OpenForge
---------------


### Activate Python Environment (Unix):

    source ./python-env/bin/activate

### Activate Python Environment (Windows):

    .\python-env\Scripts\activate.bat

### Creating a new extension:

Create a directory for your extension, change into it and then run:

    forge-extension create

### Building an extension:

    forge-extension build <platform>

Where `<platform>` is one of `chrome`, `safari`, `firefox` or `ie`

### Packaging an extension:

    forge-extension package <platform>

Where `<platform>` is one of `chrome`, `safari`, `firefox` or `ie`



Testing OpenForge
-----------------

### Generate test suites:

    forge-module-test-app -o ~/forge-workspace/v2automated/src -t automated all
    forge-module-test-app -o ~/forge-workspace/v2interactive/src -t interactive all

<!-- Link -->
   [intro-blog]: http://trigger.io/cross-platform-application-development-blog/2013/09/10/introducing-openforge-an-open-source-cross-platform-browser-add-on-framework/
   [docs]: http://legacy-docs.trigger.io/en/v1.4/modules/browser/index.html

Changing C++ libraries
---------------------
### Windows

* Boost: Download and install boost_1_57_0-msvc-12.0-32.exe from http://sourceforge.net/projects/boost/files/boost-binaries/1.57.0/
* Open .\ie\msvc\ForgeIE.sln
* For every project in solution add include dir(e.g. C:\Cpp\boost_1_57_0) and library dir(e.g. C:\Cpp\boost_1_57_0\lib32-msvc-12.0)
