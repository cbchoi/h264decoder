import libh264decoder
import numpy as np
import cv2

f_str = "sample/source_{0:03d}.bin"
decoder = libh264decoder.H264Decoder()

for i in range(251):
	print(f_str.format(i))
	f = open(f_str.format(i), "rb")
	
	binary_data = f.read()	
	frames = decoder.decode(binary_data)
	
	for framedata in frames:
	    (frame, w, h, ls) = framedata
	    
	    if frame is not None:
	        print ('frame size %i bytes, w %i, h %i, linesize %i' % (len(frame), w, h, ls))
	        frame = np.fromstring(frame, dtype=np.ubyte, count=len(frame), sep='')
	        frame = (frame.reshape((h, int(ls / 3), 3)))
	        frame = frame[:, :w, :]
	        cv2.imwrite(f_str.format(i) + ".png", cv2.cvtColor(frame, cv2.COLOR_RGB2BGR))

	f.close()
