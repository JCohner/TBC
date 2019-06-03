#for debugging
import pdb
#for delays 
import time

#Serial Initialization
import serial
ser = serial.Serial()
ser.baudrate = 115200
ser.port = '/dev/tty.usbserial-DN062JGN' #Rename this to whatever port the device shows up as
ser.open() #OPEN THIS FOR REAL TEST

##MATLAB Initialization #OLD!
# import matlab.engine
# name = matlab.engine.find_matlab()
# try:
# 	eng = matlab.engine.connect_matlab(name[0])
# except:
# 	raise Exception('Could not find shared instance of MATLAB, check documentation on sharing MATLAB instance\n')

#Alternative/Preferred MATLAB opening seq
import matlab.engine
eng = matlab.engine.start_matlab('-desktop')
try:
	eng.merlet_ui_v3() #this opens UI and shares matlab instance
except:
	pass
if (eng.matlab.engine.isEngineShared()):
	print("Connected to running instance of MATLAB!\n")
else:
	print("Oh no failed to connect")
eng.addpath('mr')
eng.addpath('matrix')
#eng.addpath('Desktop/EECS495_Robot_Studio/final_review/TBC/software')

# #intialize workspace variables
motor_values = []
eng.workspace['leg_1_value'] = 0
eng.workspace['leg_2_value'] = 0
eng.workspace['leg_3_value'] = 0
eng.workspace['leg_4_value'] = 0
eng.workspace['leg_5_value'] = 0
eng.workspace['leg_6_value'] = 0
eng.workspace['mode'] = 'False'
eng.workspace['connect'] = 0

#print("dynamic_matrix:",dynamic_matrix)
#eng.eval("load('matrix/demo_dynamic_matrix.mat')")
mode = 'False'
connect = 0
doOnce = 1

motor_values.append(('leg_1_value', eng.workspace['leg_1_value'], 0))
motor_values.append(('leg_2_value', eng.workspace['leg_2_value'], 1))
motor_values.append(('leg_3_value', eng.workspace['leg_3_value'], 2))
motor_values.append(('leg_4_value', eng.workspace['leg_4_value'], 3))
motor_values.append(('leg_5_value', eng.workspace['leg_5_value'], 4))
motor_values.append(('leg_6_value', eng.workspace['leg_6_value'], 5))

#wait untill user presses 'Begin Testing Button in Matlab'
while(connect == 0):
	if (doOnce):
		print("Press Start Testing to begin!\n")
		doOnce = 0

	connect = eng.workspace['connect']

doOnce = 1		

#begin specific test mode
while (bool(connect)):
	mode = eng.workspace['mode']
	#wait until user prepares a specific type of test
	if(mode == 'False'):
		if (doOnce):
			print("Prepare Desired Testing Mode!\n")
			doOnce = 0
		mode = eng.workspace['mode']
		if (mode != 'False'):
			print(str(mode) + " selected!")

	#Static Test
	if((bool(connect)) & (mode == 'static')):
		if (doOnce):
			print("Prepared to perform static test!\n")
			print("Press begin to begin test\n")

			doOnce = 0

		mode = eng.workspace['mode']
		connect = eng.workspace['connect']
		static_test_state = eng.workspace['static_test_state']
		if (static_test_state):
			#TODO: write function that checks all motor values
			if (motor_values[0][1] != eng.workspace['leg_1_value']):
				for motor in motor_values:				
					motor_values[motor[2]] = (motor[0], eng.workspace[motor[0]], motor[2])
					motor = motor_values[motor[2]] 
					
					ser.write(str(int(motor[1])).encode())
					ser.write(b' ')
					

					print(motor[2])
					#print(" ")
					print(int(motor[1]))
				ser.write(b'\n')
			static_test_state = eng.workspace['static_test_state']
			if not static_test_state:
				print("Static Test Ended, Prepare Different Mode if Desired")

	#Dynamic Test
	if((bool(connect)) & (mode == 'dynamic')):
		mode = eng.workspace['mode']
		connect = eng.workspace['connect']
		#print("we jump into dynamic test")
		dynamic_test_state = eng.workspace['dynamic_test_state']
		dynamic_demo_flag =eng.workspace['dynamic_demo_flag']
		#print("dynamic_demo_flag:",dynamic_demo_flag)
		if dynamic_demo_flag==1:
				#matrix = eng.workspace['demo_dynamic_matrix']
				matrix=eng.load('demo_dynamic_matrix.mat')
				#load as a dict, only grab the array part
				matrix=matrix['demo_dynamic_matrix']
				print("get the matrix")
				print("matrix[0][0] is")
				print(matrix[0][0])
				for i in range(len(matrix)):
					for j in range(6):

						# ser.write(str(j).encode())
						# ser.write(b' ')
						# ser.write(str(matrix[i][j]).encode())
						# ser.write(b'\n')
						ser.write(str(int(matrix[i][j])).encode())
						ser.write(b' ')

						print(str(j).encode())
						print(b' ')
						print(str(int(matrix[i][j])).encode())
						print(b'\n')
					ser.write(b'\n')
					time.sleep(0.2)	
				#set flag to 0 after execute demo dynamic
				eng.workspace['dynamic_demo_flag'] = 0

		if dynamic_demo_flag==2:
			#matrix = eng.workspace['demo_dynamic_matrix']
			matrix=eng.load('demo2_dynamic_matrix.mat')
			#load as a dict, only grab the array part
			matrix=matrix['demo2_dynamic_matrix']
			print("get the demo2 matrix")
			print("demo2 matrix length is",len(matrix))
			#print("matrix[0][0] is")
			print(matrix[0][0])
			for i in range(len(matrix)):
				print("now it's waypoint:",i)
				for j in range(6):

					# ser.write(str(j).encode())
					# ser.write(b' ')
					# ser.write(str(matrix[i][j]).encode())
					# ser.write(b'\n')
					ser.write(str(int(matrix[i][j])).encode())
					ser.write(b' ')

					# print(str(j).encode())
					# print(b' ')
					# print(str(int(matrix[i][j])).encode())
					# print(b'\n')
				ser.write(b'\n')
				time.sleep(0.1)	
			#set flag to 0 after execute demo dynamic
			eng.workspace['dynamic_demo_flag'] = 0

		if (dynamic_test_state):
			matrix = eng.workspace['dynamic_matrix']
			for i in range(len(matrix)):
				for j in range(6):

					# ser.write(str(j).encode())
					# ser.write(b' ')
					# ser.write(str(matrix[i][j]).encode())
					# ser.write(b'\n')

					print(str(j).encode())
					print(b' ')
					print(str(matrix[i][j]).encode())
					print(b'\n')
				time.sleep(0.5)
			#set flag to 0 after execute one direction dynamic test
			eng.workspace['dynamic_test_state'] = 0

	# #Foward Kinematics Test
	# if((bool(connect)) & (mode == 'fk')):
	# 	fk_test_state = eng.workspace['fk_test_state']
	# 	if(fk_test_state):
	# 		if (if_any_value_changed()):
	# 			for motor in motor_values:				
	# 				motor_values[motor[2]] = (motor[0], eng.workspace[motor[0]], motor[2])
	# 				motor = motor_values[motor[2]] 
					
	# 				ser.write(str(int(motor[1])).encode())
	# 				ser.write(b' ')
					

	# 				print(motor[2])
	# 				#print(" ")
	# 				print(int(motor[1]))
	# 			ser.write(b'\n')


print('program end!')
#eng.quit()




# def if_any_value_changed()
# 	if (motor_values[0][1] != eng.workspace['leg_1_value'] ||
# 	    motor_values[1][1] != eng.workspace['leg_2_value'] ||
# 		motor_values[2][1] != eng.workspace['leg_3_value'] ||
# 		motor_values[3][1] != eng.workspace['leg_4_value'] ||
# 		motor_values[4][1] != eng.workspace['leg_5_value'] ||
# 		motor_values[5][1] != eng.workspace['leg_6_value'] ||):
# 		return True
# 	else:
# 		return False







