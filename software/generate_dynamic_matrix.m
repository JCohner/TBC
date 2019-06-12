%function generate_dynamic_matrix
%Description:
%   Generate a series waypoints of leg lengths given the amplitude and frequency.
%Args:
%   w_index: determine which parameter of twist to change, for dynamic
%   test, each time can only change one degree.
%
%   Dynamic_var_A: The amplitude of sin wave. For first 3 parameters(roll,
%   pitch, yaw), the range is (0,0.3), for last 3 parameters(x,y,z), the
%   range is (0,1), all in centimeters.
%
%   Dynamic_var_Hz: The frequency of sin wave, As specifics suggests, it
%   shoulb lower than 3Hz.
%Returns:
%   dynamic_matrix: waypoints_num*6 matrix, each row represents the six
%   legs' positions given one waypoint. This matrix is used by
%   communication.py to send leg positions to tiva.
%   
%   dynamic_matrix_vel: waypoints_num*6 matrix, each row represents the six
%   legs' velocity given one waypoint. This matrix is used by
%   communication.py to send leg velocity to tiva.
%
%   dynamic_matrix_new_p: 3*6*waypoints_num 3d matrix, given each waypoint,
%   a 3*6 2d matrix represents positions of six platform joints. Only for plot
%   use
%
%   dynamic_matrix_b: 3*6*waypoints_num 3d matrix, given each waypoint,
%   a 3*6 2d matrix represents positions of six base joints. Only for plot use
%
%   dynamic_matrix_l_new: 6*waypoints_num matrix, each column represents the six
%   legs' positions given one waypoint. Only for plot use
function [dynamic_matrix,dynamic_matrix_vel,dynamic_matrix_new_p,dynamic_matrix_b,dynamic_matrix_l_new,waypoints_num]...
    =generate_dynamic_matrix(w_index,Dynamic_var_A,Dynamic_var_Hz)
A=Dynamic_var_A;
f=Dynamic_var_Hz;
T=1/f;
%No matter how frequency changes, for one period, only sample 100 points. 
%waypoints_num=T*16;
waypoints_num=100;
time_interval=T/waypoints_num;
%dynamic_matrix and dynamic_matrix_vel first to be 6*waypoints_num, transpose them at last 
dynamic_matrix=zeros(6,waypoints_num);
dynamic_matrix_vel=zeros(6,waypoints_num);
%dynamic_matrix_l_new is same as dynamic_matrix without transpose, for plot
%use.
dynamic_matrix_l_new=dynamic_matrix;
%3*6*waypoints_num 3d matrix, store corresponding six joint's position
new_p=zeros(3,6);
dynamic_matrix_new_p=repmat(new_p,1,1,waypoints_num);
dynamic_matrix_b=dynamic_matrix_new_p;
prev_l_new=[1,1,1,1,1,1]*15000;
inputs=[0;0;0;0;0;0];
for t=1:waypoints_num
    inputs(w_index) =  A * sin(2*pi*t/waypoints_num);
    inputs(2) =  A * cos(2*pi*t/waypoints_num);
    [new_p, b, l_new] = merlet_ik(inputs);
    dynamic_matrix_l_new(:,t)=l_new;
    dynamic_matrix_new_p(:,:,t)=new_p;
    dynamic_matrix_b(:,:,t)=b;
    %real position should be 15000 up.
    l_new=l_new+15000;
    dynamic_matrix(:,t)=l_new;
    dynamic_matrix_vel(:,t)=(l_new-prev_l_new)/time_interval;
    prev_l_new=l_new;    
end
%6*waypoints_num matrix: store each legs position
dynamic_matrix_l_new=dynamic_matrix;
%waypoints*6 matrix, transposing is for convenience of python file
dynamic_matrix = transpose(dynamic_matrix);
dynamic_matrix_vel = transpose(dynamic_matrix_vel);
end