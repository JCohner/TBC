function [dynamic_matrix,dynamic_matrix_new_p,dynamic_matrix_b,dynamic_matrix_l_new]...
    =generate_dynamic_matrix(Dynamic_var_A,Dynamic_var_Hz)
A=Dynamic_var_A;
f=Dynamic_var_Hz;
T=1/f;
%T=1;
w=2*pi/T;
waypoints_num =1000;

dynamic_matrix=zeros(6,waypoints_num);
dynamic_matrix_l_new=dynamic_matrix;
%3*6*waypoints_num 3d matrix, store corresponding six joint's position
new_p=zeros(3,6);
dynamic_matrix_new_p=repmat(new_p,1,1,waypoints_num);
dynamic_matrix_b=dynamic_matrix_new_p;
inputs=[0;0;0;0;0;0];
for t=1:waypoints_num
    roll =  A * sin(2*pi*t/waypoints_num);
    inputs(1)= roll;
    %pitch =  A * cos(2*pi*t/waypoints_num);
    %inputs(2)= pitch;
    [new_p, b, l_new] = merlet_ik(inputs);
    dynamic_matrix_l_new(:,t)=l_new;
    dynamic_matrix_new_p(:,:,t)=new_p;
    dynamic_matrix_b(:,:,t)=b;
    %real position should be 15000 up.
    l_new=l_new+15000;
    dynamic_matrix(:,t)=l_new;
    
end
%6*waypoints_num matrix: store each legs position
dynamic_matrix_l_new=dynamic_matrix;
%waypoints*6 matrix, transposing is for convenience of python file
dynamic_matrix = transpose(dynamic_matrix);
end