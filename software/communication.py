#for debugging
import pdb
#for delays 
import time

#Serial Initialization
import serial
ser = serial.Serial()
ser.baudrate = 115200
ser.port = 'COM3' #Rename this to whatever port the device shows up as
#ser.open() #OPEN THIS FOR REAL TEST

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
	#wait until user prepares a specific type of test
	while(mode == 'False'):
		if (doOnce):
			print("Prepare Desired Testing Mode!\n")
			doOnce = 0
		mode = eng.workspace['mode']
		if (mode != 'False'):
			print(str(mode) + " selected!")

	while((bool(connect)) & (mode == 'static')):
		if (doOnce):
			print("Prepared to perform static test!\n")
			print("Press begin to begin test\n")

			doOnce = 0

		mode = eng.workspace['mode']
		connect = eng.workspace['connect']
		static_test_state = eng.workspace['static_test_state']
		while (static_test_state):
			for motor in motor_values:
				if (motor[1] != eng.workspace[motor[0]]):
					motor_values[motor[2]] = (motor[0] ,eng.workspace[motor[0]], motor[2])
					motor = motor_values[motor[2]] 
					
					#ser.write(str(motor[2]).encode())
					#ser.write(b' ')
					# if (int(motor[2]) == 5 & int(motor[1]) < 100):
					# 	motor[1] = 100	
					#ser.write(str(motor[1]).encode())
					#ser.write(b'\n')

					print(motor[2])
					#print(" ")
					print(motor[1])
			static_test_state = eng.workspace['static_test_state']
			if not static_test_state:
				print("Static Test Ended, Prepare Different Mode if Desired")



print('program end!')