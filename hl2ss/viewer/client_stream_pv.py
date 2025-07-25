from pynput import keyboard

import cv2
import hl2ss_imshow
import hl2ss
import hl2ss_lnm
import hl2ss_utilities

# Settings --------------------------------------------------------------------

# HoloLens address
host = "10.29.211.183"

# Camera parameters
width     = 1920
height    = 1080
framerate = 30

#------------------------------------------------------------------------------

operating_mode = hl2ss.StreamMode.MODE_1
video_profile = hl2ss.VideoProfile.H265_MAIN
divisor = 1
bitrate = hl2ss_lnm.get_video_codec_default_bitrate(width, height, framerate, divisor, video_profile)

listener = hl2ss_utilities.key_listener(keyboard.Key.esc)
listener.open()

codec = hl2ss.decode_pv(video_profile)

client = hl2ss._gatherer()
client.open(host, hl2ss.StreamPort.PERSONAL_VIDEO, hl2ss_lnm.create_sockopt(), hl2ss.ChunkSize.PERSONAL_VIDEO, operating_mode)
configuration = bytearray()
configuration.extend(hl2ss._create_configuration_for_mode(hl2ss._PVCNT.START | operating_mode))
configuration.extend(hl2ss._create_configuration_for_mrc_video(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
configuration.extend(hl2ss._create_configuration_for_video_format(width, height, framerate))
configuration.extend(hl2ss._create_configuration_for_video_divisor(divisor))
configuration.extend(hl2ss._create_configuration_for_video_encoding(video_profile, hl2ss.H26xLevel.DEFAULT, bitrate))
configuration.extend(hl2ss._create_configuration_for_h26x_encoding({hl2ss.H26xEncoderProperty.CODECAPI_AVEncMPVGOPSize : framerate}))
client.sendall(bytes(configuration))

while (not listener.pressed()):
    data = client.get_next_packet()    
    data.payload = codec.decode(data.payload, 'bgr24')
    cv2.imshow('Video', data.payload.image)
    cv2.waitKey(1)

client.close()
