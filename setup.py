from setuptools import setup


setup(
    name = 'p4c_behavioral',
    version = '0.9.4',
    install_requires=['pyyaml'],
    packages=['p4c_bm', 'p4c_bm/util', 'p4c_bm/tenjin',],
    include_package_data = True,
    package_data = {
        'p4c_bm' : ['*.json'],
    },
    entry_points = {
        'console_scripts': [
            'p4c-behavioral=p4c_bm.shell:main',
            'p4c-bm=p4c_bm.shell:main',
        ],
    },
    author = 'Antonin BAS',
    author_email = 'antonin@barefootnetworks.com',
    description = 'p4c-behavioral: P4 compiler for the behavioral model target',
    license = '',
    url = 'http://www.barefootnetworks.com/',
)
