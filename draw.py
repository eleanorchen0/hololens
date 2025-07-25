import cv2 as cv
import numpy as np

blank = np.zeros((500, 500, 3), dtype='uint8')

blank[200:300, 300:400] = 255, 0, 0

cv.rectangle(blank, (0,0), (250,250), (0,0,255), thickness=2)

cv.circle(blank, (blank.shape[1]//2, blank.shape[0]//2), 40, (0,255,0), thickness=-1)


cv.line(blank, (0,0), (blank.shape[1]//2, blank.shape[0]//2), (255,255,255), thickness=2)


cv.putText(blank, 'Hi', (225,225), cv.FONT_HERSHEY_TRIPLEX, 1.0, (255,255,255), 2)
cv.imshow('text', blank)

cv.waitKey(0)