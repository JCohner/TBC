#for debugging
import pdb
#for delays 
import time
import scipy.io

#mat = scipy.io.loadmat('./matrix/demo_dynamic_matrix.mat')
#mat=mat['demo_dynamic_matrix']
#print(mat.shape)
#print(mat[0,0])
#Alternative/Preferred MATLAB opening seq
import matlab.engine
eng = matlab.engine.start_matlab('-desktop')
eng.addpath('mr')
eng.addpath('matrix')
#eng.addpath('Desktop/EECS495_Robot_Studio/final_review/TBC/software')

matrix=eng.load('demo_dynamic_matrix.mat')
print(matrix)
matrix=matrix['demo_dynamic_matrix']
print(matrix[0][0])
print(len(matrix))
