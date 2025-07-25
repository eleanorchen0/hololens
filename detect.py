import cv2 as cv

img = cv.imread('Photos/faces.jpg')

grey = cv.cvtColor(img, cv.COLOR_BGR2GRAY)

haarCascade = cv.CascadeClassifier('haar_face.xml')

facesRect = haarCascade.detectMultiScale(grey, scaleFactor=1.1, minNeighbors=6)

print('number of faces found = ', len(facesRect))

for (x,y,w,h) in facesRect:
    cv.rectangle(img, (x,y),(x+w, y+h), (255,0,0), thickness = 2)

cv.imshow('Detected Faces', img)

cv.waitKey(0)