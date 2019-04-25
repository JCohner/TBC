import pdb


#Seral Initialization
import serial
ser = serial.Serial()
ser.baudrate = 115200
ser.port = 'COM15'
ser.open()

#MATLAB Initialization
import matlab.engine
name = matlab.engine.find_matlab()
eng = matlab.engine.connect_matlab(name[0])
print("matlab found w name: " + str(name[0]))

XPosValue = eng.workspace['XPosValue']
testStatus = eng.workspace['selectedButton']

sent_messages = []

print(testStatus)
message = 0 #make sure message is defined before its called

#pdb.set_trace()

while(testStatus != "Cancel Test"):
	testStatus = eng.workspace['selectedButton']
	#print(testStatus)
	if ((XPosValue != eng.workspace['XPosValue']) & (testStatus == "Begin Test")):
		XPosValue = eng.workspace['XPosValue']
		message = str(XPosValue)[:7]
		sent_messages.append(message)
		print("XPosValue changed to: " + message)

		ser.write(message.encode())

	if ((testStatus == "Prepare Test") & (len(sent_messages) != 0)):
		#pdb.set_trace()
		sent_messages.pop(0)
		received = ser.read(len(message.encode()))
		print('Message received: ' + received.decode())
