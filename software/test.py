#for debugging
import pdb
#for delays 
import time
import scipy.io

import matlab.engine
eng = matlab.engine.start_matlab('-desktop')
eng.addpath('mr')
eng.addpath('matrix')

matrix=eng.load('demo_dynamic_matrix.mat')
matrix=matrix['demo_dynamic_matrix']

for i in range(len(matrix)):
    for j in range(6):

        # ser.write(str(j).encode())
        # ser.write(b' ')
        # ser.write(str(matrix[i][j]).encode())
        # ser.write(b'\n')
        #ser.write(str(int(matrix[i][j])).encode())
        #ser.write(b' ')

        print(str(j).encode())
        print(b' ')
        print(str(int(matrix[i][j])).encode())
        print(b'\n')
    #ser.write(b'\n')
    time.sleep(0.5)
	
#print(matrix[0][0])
#print(len(matrix))
