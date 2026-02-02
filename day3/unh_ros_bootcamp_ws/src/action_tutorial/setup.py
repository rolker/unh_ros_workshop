from setuptools import find_packages, setup
import os
from glob import glob

package_name = 'action_tutorial'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'config'), glob('config/*.rviz')),
        (os.path.join('share', package_name, 'launch'), glob('launch/*.launch.py')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='olagh48652',
    maintainer_email='olaghattas@hotmail.com',
    description='TODO: Package description',
    license='TODO: License declaration',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            'follow_full_trajectory_client = action_tutorial.follow_full_trajectory_client:main',
            'follow_full_trajectory_server = action_tutorial.follow_full_trajectory_server:main',
            'follow_full_trajectory_server_multithread = action_tutorial.follow_full_trajectory_server_multithread:main',
        ],
    },
)
