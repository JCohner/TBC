var staticData, outputTable, container, beginTest, xVal, yVal, zVal, rollVal, pitchVal, yawVal, t1Val, t2Val, x_container, y_container, z_container, roll_container, pitch_container, yaw_container, t1_container, t2_container;

function setup(){
	container = document.getElementById('output');
	outputTable = document.getElementById('outputTable');
	x_container = document.getElementById('xpos');
	y_container = document.getElementById('ypos');
	z_container = document.getElementById('zpos');
	roll_container = document.getElementById('rollval');
	pitch_container = document.getElementById('pitchval');
	yaw_container = document.getElementById('yawval');
	t1_container = document.getElementById('t1Val');
	t2_container = document.getElementById('t2Val');
	
	beginTest = document.getElementById('beginTest');
	cancelTest = document.getElementById('cancelTest');
	
	beginTest.addEventListener('click',getValues);
	
}

function draw(){
	
}

function Data(xVal, yVal, zVal, rollVal, pitchVal, yawVal, t1Val, t2Val){
	this.x = xVal;
	this.y = yVal;
	this.z = zVal;
	this.roll  = rollVal;
	this.pitch = pitchVal;
	this.yaw = yawVal;
	this.t1 = t1Val;
	this.t2 = t2Val;
}

function getValues(){
	console.log("you're a pimp");
	//Grab values from children nodes of input
	xVal = x_container.value;
	yVal = y_container.value;
	zVal = z_container.value;
	rollVal = roll_container.value;
	pitchVal = pitch_container.value;
	yawVal = yaw_container.value;
	t1Val = t1_container.value;
	t2Val = t2_container.value;
	
	staticData = new Data(xVal, yVal, zVal, rollVal, pitchVal, yawVal, t1Val, t2Val);
	console.log(staticData);
	
	showValues();
}


function showValues(){
	for (guy in staticData){
		var newEl = document.createElement('td');
		if (staticData[guy] != ''){
			newEl.innerHTML = staticData[guy];	
		} else {
			newEl.innerHTML = 0;
		}
		
		console.log("trying to get el w ID " + guy + '\n' + "with val " + staticData[guy]);
		var tableRow = document.getElementById(guy);
		tableRow.appendChild(newEl);
	}
	
}