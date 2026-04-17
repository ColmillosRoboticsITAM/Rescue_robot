from setuptools import find_packages, setup

package_name = 'colmibot_controller'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),

        ('share/' + package_name + '/launch', ['launch/teleop_traction.launch.py']),
        ('share/' + package_name + '/config', [
            'config/joy_config.yaml',
            'config/joy_teleop.yaml'
    ]),

    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='sergiohs',
    maintainer_email='i.sergio.hersa@gmail.com',
    description='TODO: Package description',
    license='TODO: License declaration',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
    'console_scripts': [
        'joy2traction = colmibot_controller.joy2traction:main',
        'serial_entropy = colmibot_controller.serial_entropy:main',
        ],
    },
)
