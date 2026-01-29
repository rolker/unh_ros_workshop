from setuptools import setup

package_name = 'bootcamp_vision'

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
            'camera_pub = bootcamp_vision.camera_pub:main',
            'vision_node = bootcamp_vision.vision_node:main',
        ],
    },
)
