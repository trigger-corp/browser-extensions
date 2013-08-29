
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

    git clone https://github.com/trigger-corp/browser-extensions.git browser-extensions.git

### Create Python Environment:

    cd browser-extensions.git
    virtualenv --no-site-packages ./python-env

### Activate Python Environment (Unix):

    source ./python-env/bin/activate

### Activate Python Environment (Windows):

    .\python-env\Scripts\activate.bat

### Setup Python Environment:

    cd generate
    pip install -r requirements.txt
    python setup.py develop
  


Using Forge
-----------

### Activate Python Environment (Unix):

    source ./python-env/bin/activate

### Activate Python Environment (Windows):

    .\python-env\Scripts\activate.bat

### Creating a new extension:

Create a directory for your extension, change into it and then run:

    forge create

### Building an extension:

    forge build <platform>
    
Where `<platform>` is one of `chrome`, `safari`, `firefox` or `ie`

### Packaging an extension:

    forge package <platform>

Where `<platform>` is one of `chrome`, `safari`, `firefox` or `ie`



Testing Forge
-------------

### Generate test suites:

    forge-module-test-app -o ~/forge-workspace/v2automated/src -t automated all
    forge-module-test-app -o ~/forge-workspace/v2interactive/src -t interactive all
