import pdb
import time
import numpy as np
import serial
import matlab.engine

class matlab_to_tiva():
	def __init__(self):
		#init serial port
		# self.ser = serial.Serial()
		# self.ser.baudrate = 115200
		# self.ser.port = '/dev/tty.usbserial-DN062JGN' #Rename this to whatever port the device shows up as
		# self.ser.open() #OPEN THIS FOR REAL TEST

		#start matlab and init matlab working space
		self.eng = matlab.engine.start_matlab('-desktop')
		try:
			self.eng.merlet_ui_v3() #this opens UI and shares matlab instance
		except:
			pass
		if (self.eng.matlab.engine.isEngineShared()):
			print("Connected to running instance of MATLAB!\n")
		else:
			print("Oh no failed to connect")
		self.eng.addpath('mr')
		self.eng.addpath('matrix')
		#eng.addpath('Desktop/EECS495_Robot_Studio/final_review/TBC/software')

		# #intialize workspace variables
		self.eng.workspace['leg_1_value'] = 0
		self.eng.workspace['leg_2_value'] = 0
		self.eng.workspace['leg_3_value'] = 0
		self.eng.workspace['leg_4_value'] = 0
		self.eng.workspace['leg_5_value'] = 0
		self.eng.workspace['leg_6_value'] = 0
		self.eng.workspace['mode'] = 'False'
		self.eng.workspace['connect'] = 1

	def run(self):
		leg_length=self.read_leg_length()
		connect = self.eng.workspace['connect']
		while(connect):			
			if(bool(connect)):
				mode = self.eng.workspace['mode']
				if(mode == 'static'):
					begin_test_state = self.eng.workspace['begin_test_state']
					if(begin_test_state==1):
						new_leg_length=self.read_leg_length()
						#som leg length changed
						if(np.array_equal(leg_length, new_leg_length)):
							self.move(new_leg_length)
				elif(mode == 'dynamic'):
					begin_test_state = self.eng.workspace['begin_test_state']
					if(begin_test_state==1):
						dynamic_demo_flag=self.eng.workspace['dynamic_demo_flag']
						dynamic_one_degree_flag=self.eng.workspace['dynamic_one_degree_flag']
						if (dynamic_demo_flag==1):
							#load a dict contains matrix and time etc,here only grab the array part
							matrix=self.eng.load('demo_dynamic_matrix.mat')
							matrix=matrix['demo_dynamic_matrix']
							#transform form data structure ml.double to np.ndarray, shape of matrix is N*6 
							matrix=np.asarray(matrix)
							for i in range(matrix.shape[0]):
								if(self.if_cancel()):
									break
								else:
									self.move(matrix[i,:])
									time.sleep(0.2)
							#set flag to 0 after execute demo dynamic
							self.eng.workspace['dynamic_demo_flag'] = 0
						if (dynamic_demo_flag==2):
							#load a dict contains matrix and time etc,here only grab the array part
							matrix=self.eng.load('demo2_dynamic_matrix.mat')
							matrix=matrix['demo2_dynamic_matrix']
							matrix=np.asarray(matrix)
							for i in range(matrix.shape[0]):
								if(self.if_cancel()):
									break
								else:
									self.move(matrix[i,:])
									time.sleep(0.2)	
							self.eng.workspace['dynamic_demo_flag'] = 0
						if (dynamic_one_degree_flag==1):
							matrix=self.eng.workspace['dynamic_matrix']
							matrix_vel=self.eng.workspace['dynamic_matrix_vel']
							matrix=np.asarray(matrix)
							matrix_vel=np.asarray(matrix_vel)
							for i in range(matrix.shape[0]):
								if(self.if_cancel()):
									break
								else:								
									leg_length=matrix[i,:]
									vel=matrix_vel[i,:]
									self.move(leg_length,vel)
							self.eng.workspace['dynamic_one_degree_flag'] = 0

				elif(mode == 'waypoint'):
					begin_test_state = self.eng.workspace['begin_test_state']
					if(begin_test_state==1):
						waypoints_matrix_flag=self.eng.workspace['waypoints_matrix_flag']
						if (waypoints_matrix_flag==1):
							matrix=self.eng.workspace['waypoints_matrix']
							matrix=np.asarray(matrix)
							for i in range(matrix.shape[0]):
								if(self.if_cancel()):
									break
								else:								
									leg_length=matrix[i,0:5]
									vel=matrix[i,6:11]
									t2=matrix[i,12]
									self.move(leg_length,vel)
									time.sleep(t2)
							self.eng.workspace['waypoints_matrix_flag'] = 0
				elif (mode == 'motor'):
					begin_test_state = self.eng.workspace['begin_test_state']
					if(begin_test_state==1):
						new_leg_length=self.read_leg_length()
						if(np.array_equal(leg_length, new_leg_length)):
							self.move(new_leg_length)
						
			#refresh every 0.2s
			time.sleep(0.2)
			connect = self.eng.workspace['connect']

	#read the matlab working space and return a np.array of shape (6,)
	def read_leg_length(self):
		leg_length=np.array([self.eng.workspace['leg_1_value'],self.eng.workspace['leg_2_value']
		,self.eng.workspace['leg_3_value'],self.eng.workspace['leg_4_value'],self.eng.workspace['leg_5_value'],self.eng.workspace['leg_6_value']])
		print("leg_length:",leg_length)
		return leg_length

	#send six leg's position with velocities 
	#Args:leg_length, vel
	def move(self,leg_length,vel=None):
		for i in range(6):
			print(int(leg_length[i]))
		# 	self.ser.write(str(int(leg_length[i])).encode())
		# 	self.ser.write(b' ')
		# self.ser.write(b'\n')

	def if_cancel(self):
		connect = self.eng.workspace['connect']
		begin_test_state = self.eng.workspace['begin_test_state']
		if (connect ==0 or begin_test_state==0):
			return True

if __name__ == '__main__':
	matlab_to_tiva1=matlab_to_tiva()
	matlab_to_tiva1.run()





