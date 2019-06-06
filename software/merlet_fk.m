% function merlet_fk
% Description:
%     Implement Newton-Raphson algorithm to solve the forward kinematics of merlet platform.
% Args:
%     leg_pose: 1*6 row vector represents six legs's positions.
%
%     initial_twist: 6*1 vector, initial twist is used as initial guess
% Returns:
%     twist: 6*1 vector, predicted twist by Newton-Raphson algorithm.
%
%     new_p(3*6 matrix): The coordinates of six joints on platform.
%     ith column represents ith joint. They are determined by input.
%               
%     b(3*6 matrix): The coordinates of six joints on base.
%     ith column represents ith joint. Once the configuration of whole
%     platform is determined, it won't change.
%          
%     l_new(1*6 row vector): The z position of six leg given current
%     input(twist).
%
%     l_relative_move(1*6 row vector): now is same as l_new.
%
%     iter_num: how many iterations algorithm takes to converge
function [twist,new_p, b, l_new,l_relative_move,iter_num]=merlet_fk(leg_pose,initial_twist)
%initialization
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
    twist= twist + inv_jacobian*err;
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
%l_new=leg_pose;
%l_relative_move=leg_pose;
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