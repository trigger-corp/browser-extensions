a = Analysis(
     [os.path.join(HOMEPATH,'support/_mountzlib.py'), os.path.join(HOMEPATH,'support/useUnicode.py'), 'python_runner_wrapper.py'],
     pathex=[os.getcwd()],
     hookspath=['./pyinstaller-hooks'],
)
pyz = PYZ(a.pure)

name = 'python_runner_%s' % sys.platform
if sys.platform.startswith("win"):
     name += '.exe'

exe = EXE( pyz,
          a.scripts,
          a.binaries,
          a.zipfiles,
          a.datas,
          name=os.path.join('dist', name),
          debug=False,
          strip=False,
          upx=True,
          console=True )
