import pdb
import time

#Seral Initialization
import serial
ser = serial.Serial()
ser.baudrate = 115200
ser.port = 'COM3'
ser.open() #OPEN THIS FOR REAL TEST

#MATLAB Initialization
import matlab.engine
name = matlab.engine.find_matlab()
eng = matlab.engine.connect_matlab(name[0])
print("matlab found w name: " + str(name[0]))

#XPosValue = eng.workspace['XPosValue']


sent_messages = []
motor_values = []
# #intialize workspace variables
eng.workspace['leg_1_value'] = 0
eng.workspace['leg_2_value'] = 0
eng.workspace['leg_3_value'] = 0
eng.workspace['leg_4_value'] = 0
eng.workspace['leg_5_value'] = 0
eng.workspace['leg_6_value'] = 0
eng.workspace['mode'] = 0
mode = 0
testStatus = 0



while (mode == 0):
	try:
		mode = eng.workspace['mode']
	except:
		print("select a mode value!")
		mode = 0
		time.sleep(0.5)

while((testStatus == 0 ) & (mode != 'dynamic')):	
	try:
		testStatus = eng.workspace['selectedButton']
	except:
		print("select a mode value!")
		mode = 0
		time.sleep(0.5)

motor_values.append(('leg_1_value', eng.workspace['leg_1_value'], 0))
motor_values.append(('leg_2_value', eng.workspace['leg_2_value'], 1))
motor_values.append(('leg_3_value', eng.workspace['leg_3_value'], 2))
motor_values.append(('leg_4_value', eng.workspace['leg_4_value'], 3))
motor_values.append(('leg_5_value', eng.workspace['leg_5_value'], 4))
motor_values.append(('leg_6_value', eng.workspace['leg_6_value'], 5))

print(motor_values)

#message = 0 #make sure message is defined before its called
#import pdb
#pdb.set_trace()

if (mode == 'dynamic'):
	print("matlab is thinking")
	matrix = eng.workspace['dynamic_matrix']
	print(matrix)
	print(len(matrix))
	for i in range(len(matrix)):
		for j in range(6):

			ser.write(str(j).encode())
			ser.write(b' ')
			ser.write(str(matrix[i][j]).encode())
			ser.write(b'\n')

			print(str(j).encode())
			print(b' ')
			print(str(matrix[i][j]).encode())
			print(b'\n')
		time.sleep(0.5)	
	#gtiprint(matrix)




while ((testStatus != "Cancel Test") & (mode == 'static')):
	testStatus = eng.workspace['selectedButton']
#	#print(testStatus)
	for motor in motor_values:
		if (motor[1] != eng.workspace[motor[0]]):
			motor_values[motor[2]] = (motor[0] ,eng.workspace[motor[0]], motor[2])
			motor = motor_values[motor[2]] 
			
			ser.write(str(motor[2]).encode())
			ser.write(b' ')
			# if (int(motor[2]) == 5 & int(motor[1]) < 100):
			# 	motor[1] = 100	
			ser.write(str(motor[1]).encode())
			ser.write(b'\n')

			print(motor[2])
			#print(" ")
			print(motor[1])
			#print('\n')
			
print("end!")
# 	if ((XPosValue != eng.workspace['XPosValue']) & (testStatus == "Begin Test")):
# 		XPosValue = eng.workspace['XPosValue']
# ##		sent_messages.append(message)
# 		print("XPosValue changed to: " + message)
# 		#write to a six element array
# 		ser.write(message.encode())
# 	if ((testStatus == "Prepare Test") & (ser.in_waiting!= 0)):
# 		#pdb.set_trace()
# 		sent_messages.pop(0)
# 		received = ser.read_until('\r\n', 20)
# 		print('Message received: ' + received.decode())
