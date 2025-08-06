
import numpy as np


position = [0.21847032, 1.1594939,  0.36602446] 
rotation = [ 0.0026726,  -0.32758072,  0.34853327,  0.04459876]
position1 = [0.21850848, 1.1516838,  0.36773315] 
rotation1 = [-0.01122225, -0.34560147,  0.337315,    0.04335355]

positions = np.array(position, position1)

average_position = positions.mean(axis=0)