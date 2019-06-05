function [twist,new_p, b, l_new,l_relative_move,iter_num]=merlet_fk(leg_pose,initial_twist)
%leg_pose 1*6 row vector
%initialization
%fprintf(leg_pose);
%leg_pose
%initial_twist = [0;0;0;0;0;0];
addpath('C:\Users\yling\OneDrive\Desktop\TBC-guoye (1)\TBC-guoye\merlet_platform\mr')
twist = initial_twist;
iter_num=0;
max_its=100;
conv_thresh = 1e-6;
l_goal = leg_pose';%[10000;10000;10000;10000;10000;10000]*4.6926;
%get initial six leg length of initial_twist
[new_p, b, l_new,l_relative_move] = merlet_ik (initial_twist);
err= l_goal-l_new';%err should be 6*1
norm_err = norm(err);

%Newton-Raphson Algorithm
while (iter_num <= max_its && (norm_err > conv_thresh))
    jacobian=generate_jacobian(twist);
    inv_jacobian= pinv(jacobian);
    S_twist = VecTose3(twist); %transfer 6x1 vector into 3x3 matrix
    T_twist = MatrixExp6(S_twist);%transfer 3x3 matrix into 4x4 transfermation
    
    T_new_twist = T_twist * (inv_jacobian*err); %new 4x4 transferamtion matrix
    S_new_twist = MatrixLog6(T_new_twist); %new se(3) matrix
    twist = se3ToVec(S_new_twist); %updated twist
    [new_p, b, l_new,l_relative_move] = merlet_ik (twist);
    err = l_goal-l_new';
    norm_err = norm(err,1);
    %norm_err
    iter_num = iter_num+1;
end
%get the final twist, then using ik to get new_p,b
% twist
% iter_num
[new_p, b, l_new,l_relative_move] = merlet_ik (twist);
l_new=leg_pose;
l_relative_move=leg_pose;
end

function jacobian=generate_jacobian(twist)
    perturbation = 0.0001;
    jacobian = zeros(6,6);
    %old length of leg l
    [new_p, b, l_old,l_relative_move] = merlet_ik (twist); 
    for i =1:6
    twist_perturbation = twist;
    twist_perturbation(i)=twist_perturbation(i)+perturbation;   
    [new_p, b, l_new,l_relative_move] = merlet_ik (twist_perturbation);
    %update jacobian
    jacobian(:,i)=((l_new-l_old)/perturbation)';
    end       
end