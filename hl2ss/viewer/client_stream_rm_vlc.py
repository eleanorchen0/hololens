import cv2
import hl2ss_imshow
import hl2ss
import hl2ss_lnm
import hl2ss_utilities
import hl2ss_3dcv

host = "10.29.211.183"

calibration_path = 'calibration'

port = hl2ss.StreamPort.RM_VLC_RIGHTFRONT

mode = hl2ss.StreamMode.MODE_1

profile = hl2ss.VideoProfile.H265_MAIN
bitrate = None

client = hl2ss_lnm.rx_rm_vlc(host, port, mode=mode, profile=profile, bitrate=bitrate)
client.open()

while True:
    data = client.get_next_packet()


    cv2.imshow('Video', data.payload.image)
    cv2.waitKey(1)

client.close()
