from setuptools import setup, find_packages
import sys, os

version = '1.0'

setup(name='generate',
    version=version,
    description="",
    long_description="""\
    """,
    classifiers=[], # Get strings from http://pypi.python.org/pypi?%3Aaction=list_classifiers
    keywords='',
    author='James Brady',
    author_email='james@trigger.io',
    url='https://trigger.io/',
    license='Proprietary',
    packages=find_packages(exclude=['ez_setup', 'examples', 'tests']),
    include_package_data=True,
    zip_safe=False,
    install_requires=[
    ],
    entry_points={
        'console_scripts': [
            'forge = build_tools.main:main',
            'forge-generate = generate.main:main',
            'forge-module-test-app = internal.generate_module_test_app:main'
        ]
    }
)
