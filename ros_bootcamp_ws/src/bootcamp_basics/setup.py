from setuptools import setup

package_name = 'bootcamp_basics'

setup(
    name=package_name,
    version='0.0.1',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages', ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools',],
    zip_safe=True,
    maintainer='bootcamp',
    maintainer_email='bootcamp@example.com',
    description='ROS2 Bootcamp package',
    license='Apache-2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'talker = bootcamp_basics.talker:main',
            'listener = bootcamp_basics.listener:main',
            'add_three_ints_srv = bootcamp_basics.add_three_ints_srv:main',
            'add_three_ints_client = bootcamp_basics.add_three_ints_client:main',
            'turtlesim_square = bootcamp_basics.turtlesim_square:main',
        ],
    },
)
