import matlab.engine

#python functions for matlab engine
	#matlab.engine.find_matlab()
	#matlab.engine.connect_matlab(name)
		#connects to shared matab session, returns MatlabEngine object
		#name species name of MATLAB sessionn already running on machine
		#when called w/out name just starts a new shared MATLAB session
	#matlab.engine.start_matlab()

name = matlab.engine.find_matlab()

eng = matlab.engine.connect_matlab(name[0]) #i like using the name 'ayoTown'
print(eng.workspace)

print("we're gonna add an element\n")
eng.workspace['added_Shiii'] = 'we added'

print(eng.workspace)



