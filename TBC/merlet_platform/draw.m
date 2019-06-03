
inputs_loop = zeros(6,200);
roll=-pi/9;
for i =1:200
    inputs_loop(1,i)=roll;
    if(roll>pi/9)
        roll=-pi/9;
    else
        roll=roll+pi/900;
    end
    merlet_ik(inputs_loop(:,i));
end


    