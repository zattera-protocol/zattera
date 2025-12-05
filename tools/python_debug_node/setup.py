from setuptools import setup

setup(name="zatteradebugnode",
       version="0.1",
       description="A wrapper for launching and interacting with a Zattera Debug Node",
       url="http://github.com/zattera-protocol/zattera",
       author="Zattera Core Team",
       author_email="dev@zattera.club",
       license="See LICENSE.md",
       packages=[ "zatteradebugnode", "zatteranoderpc" ],
       install_requires=[
           "websocket-client"
       ],
       zip_safe=False)
