%leg_pose=[1.4644 ,   1.4927 ,   1.5455  ,  1.5455  ,  1.4927,    1.4644 ]*10000;
% leg_pose=[1.6 ,   1.5 ,  1.5  ,  1.5  ,  1.5,    1.5 ]*10000;
% [new_p, b, l_new,l_relative_move]=merlet_fk(leg_pose);

% input=[0;0;0;0.1;0;0];
% [new_p, b, l_new,l_relative_move] = merlet_ik (input);
% l_new

%twist is column
fk_twist=zeros(6,100);
t=13.8;
waypoints_num=100;
inputs=[0;0;0;0;0;0];
roll =  0.3 * sin(2*pi*t/waypoints_num);
%inputs(1)= roll;
initial_twist = inputs;
%[new_p, b, l_new] = merlet_ik(inputs);
for i=1:100
    i
    initial_twist    
    [twist,new_p, b, l_new,l_relative_move,iter_num]=merlet_fk(demo2_dynamic_matrix(i,:),initial_twist);
    twist
    iter_num
    fk_twist(:,i)=twist;
    initial_twist=twist;
end