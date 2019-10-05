H264 Decoder Python Module for Python3
==========================

The aim of this project is to decode video stream from DJI Tello using Python3.
Since the original project from DaWelter provides the features in python 2.7, 
I modified the code to work the h.265 decoding on Python 3 environment. 

Followings are the project summary of the original project.
> The aim of this project is to provide a simple decoder for video
> captured by a Raspberry Pi camera. At the time of this writing I only
> need H264 decoding, since a H264 stream is what the RPi software 
> delivers. Furthermore flexibility to incorporate the decoder in larger
> python programs in various ways is desirable.
> 
> The code might also serve as example for libav and boost python usage.
`

Files
-----
* `h264decoder.hpp`, `h264decoder.cpp` and `h264decoder_python.cpp` contain the module code.
* `h264decoder_py3_test.py` contain the python3 code that utilizes the h265decoder.
* `sample/source000.bin ~ sample/source_250.bin` contain the video stream from DJI tello
* Other source files are tests and demos.


Requirements
------------
* cmake for building
* libav
* boost python3
* python3


License
-------
The code is published under the Mozilla Public License v. 2.0. 
