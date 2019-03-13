var staticData, container, beginTest, xVal, yVal, zVal, rollVal, pitchVal, yawVal, x_container, y_container, z_container, roll_container, pitch_container, yaw_container;

function setup(){
	container = document.getElementById('output');
	x_container = document.getElementById('xpos');
	y_container = document.getElementById('ypos');
	z_container = document.getElementById('zpos');
	roll_container = document.getElementById('rollval');
	pitch_container = document.getElementById('pitchval');
	yaw_container = document.getElementById('yawval');
	
	beginTest = document.getElementById('beginTest');
	
	beginTest.addEventListener('click',getValues);
}

function draw(){
	
}

function Data(xVal, yVal, zVal, rollVal, pitchVal, yawVal){
	this.x = xVal;
	this.y = yVal;
	this.z = zVal;
	this.roll  = rollVal;
	this.pitch = pitchVal;
	this.yaw = yawVal;
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
	
	
	staticData = new Data(xVal, yVal, zVal, rollVal, pitchVal, yawVal);
	console.log(staticData);
	
	showValues();
}


function showValues(){
	var outputTable = document.createElement('table');
	output.classList.add('table');
	//output.classList.add('justify-content-center');
	
	var outputTableHead = document.createElement('thead');
	outputText.classList.add('thead-dark');
	//outputText.classList.add('rounded');
	//outputText.classList.add('text-center');
	var outputTableHeadTR = document.createElement('tr');
	outputTableHead.appendChild(outputTableHeadTR);
	for (guy in staticData){
		var outputTableHeadEL = document.createElement('th');
		outputTableHeadEL.addS
	}

	container.appendChild(output);
	output.appendChild(outputText);

	outputText.innerHTML = "Static Values Sent:";
	
	for (guy in staticData){
		console.log(staticData[guy]);
		var outputItem = document.createElement('ul');
		outputItem.classList.add('list-group');
		outputItem.classList.add('justify-content-center');
		
		var staticText = document.createElement('li');
		staticText.classList.add('list-group-item');
		staticText.classList.add('col');
		staticText.classList.add('text-center');
		staticText.innerHTML = guy;
		
		var dynamicText = document.createElement('li');
		dynamicText.classList.add('list-group-item');
		dynamicText.classList.add('col');
		dynamicText.classList.add('text-center');
		dynamicText.innerHTML = staticData[guy];
		
		container.appendChild(outputItem);
		outputItem.appendChild(staticText);
		outputItem.appendChild(dynamicText);
			
		
	}
	
}