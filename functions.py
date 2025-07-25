import cv2 as cv
import numpy as np
import matplotlib.pyplot as plt

img = cv.imread('Photos/cat.jpg')

cv.imshow('Cat', img)

def changeRes(width, height):
    #live vid
    capture.set(3, width)
    capture.set(4, height)


def rescaleFrame(frame, scale=0.75):
        width = int(frame.shape[1] * scale)
        height = int(frame.shape[0] * scale)
        dimensions = (width, height)

        return cv.resize(frame, dimensions, interpolation=cv.INTER_AREA)

capture = cv.VideoCapture('Videos/dog.mp4')

resizedImage = rescaleFrame(img)
cv.imshow('Image', resizedImage)

while True:

        isTrue, frame = capture.read()


        frame_resized = rescaleFrame(frame)

        cv.imshow('Video', frame)
        cv.imshow('Video Resized', frame_resized)

        if cv.waitKey(20) & 0xFF == ord('d'):
                break


capture.release()
cv.destroyAllWindows()

cv.waitKey(0)


image = cv.imread('Photos/cat2.jpg')
blank = np.zeros(image.shape[:2], dtype='uint8')
grey = cv.cvtColor(image, cv.COLOR_BGR2GRAY)

# EDGE DETECTION

# LAPLACIAN
lap = cv.Laplacian(grey, cv.CV_64F)
lap = np.uint8(np.absolute(lap))
cv.imshow('Laplacian', lap)

# SOBEL
sobelx = cv.Sobel(grey, cv.CV_64F, 1, 0)
sobely = cv.Sobel(grey, cv.CV_64F, 0, 1)
combined_sobel = cv.bitwise_or(sobelx, sobely)
cv.imshow('combined sobel', combined_sobel)

# # THRESHOLDING
# threshold, thresh = cv.threshold(grey, 150, 255, cv.THRESH_BINARY)
# threshold_inv, thresh_inv = cv.threshold(grey, 150, 255, cv.THRESH_BINARY_INV)
#
# # ADAPTIVE THRESHOLDING
# adaptive_thresh = cv.adaptiveThreshold(grey, 255, cv.ADAPTIVE_THRESH_MEAN_C,
#                                        cv.THRESH_BINARY, 11, 1)
#
# # GREY SCALE
#
# cv.imshow('Grey', grey)
#
# mask = cv.circle(blank.copy(), (image.shape[1]//2, image.shape[0]//2), 200, 255, -1)
# masked = cv.bitwise_and(grey, grey, mask=mask)
# cv.imshow('Masked', masked)
#
# greyHist = cv.calcHist([grey], [0], mask, [256], [0,256])
# plt.figure()
# plt.title('Greyscale Histogram')
# plt.xlabel('Bins')
# plt.ylabel('# of pixels')
# plt.plot(greyHist)
# plt.xlim([0,256])
# plt.show()
#
# # MASKING
# plt.figure()
# plt.title('Color Histogram')
# plt.xlabel('Bins')
# plt.ylabel('# of pixels')
#
# maskedColor = cv.bitwise_and(image, image, mask=mask)
# cv.imshow('Masked', maskedColor)
#
# colors = ('b','g','r')
# for i,col in enumerate(colors):
#     hist = cv.calcHist([image], [i], mask, [256], [0,256])
#     plt.plot(hist, color=col)
#     plt.xlim([0,256])
#
# plt.show()
#
# circle = cv.circle(blank.copy(), (image.shape[1]//2, image.shape[0]//2), 100, 255, -1)
# cv.imshow('circle', circle)
#
# # BITWISE
# rectangle = cv.rectangle(blank.copy(), (500,500), (800,800), 255, -1)
# cv.imshow('Rectangle', rectangle)
# mask = cv.bitwise_or(rectangle, circle)
# cv.imshow('Mask', mask)
#
# masked = cv.bitwise_and(image, image, mask=mask)
# cv.imshow('Masked', masked)
# circle = cv.circle(blank.copy(), (200,200), 200, 255, -1)
# cv.imshow('Rectangle', rectangle)
# cv.imshow('Circle', circle)
# #intersecting
# bitwiseAnd = cv.bitwise_and(rectangle, circle)
# cv.imshow('And', bitwiseAnd)
# #intersecting and non-intersecting
# bitwiseOr = cv.bitwise_or(rectangle, circle)
# cv.imshow('Or', bitwiseOr)
# #non-intersecting
# bitwiseXor = cv.bitwise_xor(rectangle, circle)
# cv.imshow('Xor', bitwiseXor)
# #none
# bitwiseNot = cv.bitwise_not(rectangle)
# cv.imshow('Not', bitwiseNot)
#
#
# # BLUR
# #higher kernel size = higher blur, kernel size can only be odd, needs center square
# average = cv.blur(image, (7,7))
# cv.imshow('Average', average)
# #better at removing noise, compared to average and gauss
# median = cv.medianBlur(image, 7)
#
# bilateral = cv.bilateralFilter(image, 5, 15, 50)
# b,g,r = cv.split(image)
#
# # COLOR
# cv.imshow('blue', b)
# cv.imshow('green', g)
# cv.imshow('red', r)
#
# print(image.shape)
# print(b.shape)
# print(g.shape)
# print(r.shape)
#
# blue = cv.merge([b,blank,blank])
# green = cv.merge([blank,g,blank])
# red = cv.merge([blank,blank,r])
#
# merged = cv.merge([b,g,r])
#
# # COLOR SPACES
# hsv = cv.cvtColor(image, cv.COLOR_BGR2HSV)
# lab = cv.cvtColor(image, cv.COLOR_BGR2LAB)
# rgb = cv.cvtColor(image, cv.COLOR_BGR2RGB)
# bgr = cv.cvtColor(grey, cv.COLOR_GRAY2BGR)
# cv.imshow('LAB', lab)
#
# plt.imshow(rgb)
# plt.show()
#
# # FUNCTIONS
# blur = cv.GaussianBlur(image, (7,7), 0)
# edgeCascade = cv.Canny(blur, 125, 175)
#
# dilated = cv.dilate(edgeCascade, (3,3), iterations=1)
# eroded = cv.erode(dilated, (3,3), iterations=1)
#
# resized = cv.resize(image, (500,500), interpolation=cv.INTER_CUBIC)
# cropped = image[50:200, 200:400]
#
# ret, thresh = cv.threshold(grey, 125, 255, cv.THRESH_BINARY)
# cv.imshow('Thresh', thresh)
#
# countors, hierarchies = cv.findContours(thresh, cv.RETR_LIST, cv.CHAIN_APPROX_SIMPLE)
# print(f'{len(countors)} contour(s) found')
# cv.drawContours(blank, countors, -1, (0,0,255), 1)
# cv.imshow('Contours', blank)
#

cv.waitKey(0)