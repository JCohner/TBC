#Seral Initialization
import serial
ser = serial.Serial()
ser.baudrate = 115200
ser.port = 'COM22'
#ser.open()

#MATLAB Initialization
import matlab.engine
name = matlab.engine.find_matlab()
eng = matlab.engine.connect_matlab(name[0])
print("matlab found w name: " + str(name[0]))

XPosValue = eng.workspace['XPosValue']
testStatus = eng.workspace['selectedButton']

print(XPosValue)
print(testStatus)

while(testStatus != "Cancel Test"):
	testStatus = eng.workspace['selectedButton']
	if (XPosValue != eng.workspace['XPosValue']):
		XPosValue = eng.workspace['XPosValue']
		print("XPosValue changed to: " + str(XPosValue))
		message = XPosValue
		#ser.write(message.encode())