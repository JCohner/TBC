import pdb


#Seral Initialization
import serial
ser = serial.Serial()
ser.baudrate = 115200
ser.port = 'COM26'
ser.open() #OPEN THIS FOR REAL TEST

#MATLAB Initialization
import matlab.engine
name = matlab.engine.find_matlab()
eng = matlab.engine.connect_matlab(name[0])
print("matlab found w name: " + str(name[0]))

XPosValue = eng.workspace['XPosValue']
testStatus = eng.workspace['selectedButton']

sent_messages = []
motor_values = []
motor_values.append(('XPosValue', eng.workspace['XPosValue'], 0))
motor_values.append(('YPosValue', eng.workspace['YPosValue'], 1))
motor_values.append(('ZPosValue', eng.workspace['ZPosValue'], 2))
motor_values.append(('rollValue', eng.workspace['rollValue'], 3))
motor_values.append(('yawValue', eng.workspace['yawValue'], 4))
motor_values.append(('pitchValue', eng.workspace['pitchValue'], 5))

print(motor_values)

#message = 0 #make sure message is defined before its called
#import pdb
#pdb.set_trace()

while (testStatus != "Cancel Test"):
	testStatus = eng.workspace['selectedButton']
#	#print(testStatus)
	for motor in motor_values:
		if (motor[1] != eng.workspace[motor[0]]):
			ser.write(str(motor[2]).encode())
			
			
			motor_values[motor[2]] = (motor[0] ,eng.workspace[motor[0]], motor[2])
			motor = motor_values[motor[2]] 
			ser.write(b' ')
			ser.write(str(motor[1]).encode())
			print(motor[2])
			#print(" ")
			print(motor[1])
			#print('\n')
			ser.write(b'\n')

#	if ((XPosValue != eng.workspace['XPosValue']) & (testStatus == "Begin Test")):
#		XPosValue = eng.workspace['XPosValue']
# ##		sent_messages.append(message)
# 		print("XPosValue changed to: " + message)
# 		#write to a six element array
# 		ser.write(message.encode())
#	if ((testStatus == "Prepare Test") & (ser.in_waiting!= 0)):
# 		#pdb.set_trace()
# 		sent_messages.pop(0)
#		received = ser.read_until('\r\n', 20)
#		print('Message received: ' + received.decode())
