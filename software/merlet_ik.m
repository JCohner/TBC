% function merlet_ik
% Description:
%     Solve the inverse kinematics given a input Twist.
% Args:
%     input(6*1 vector): A twist in order of [roll;pitch;yaw;x;y;z],x,y,z
%     in centimeters
% Returns:
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
%     l_relative_move(1*6 row vector): now is same as l_new
function [new_p, b, l_new,l_relative_move] = merlet_ik (input)
addpath('~/Desktop/EECS495_Robot_Studio/merlet/mr')
%Transfer gui x,y,z values from cm to microns.
micron_num =10000;
input(4)=input(4)*micron_num;
input(5)=input(5)*micron_num;
input(6)=input(6)*micron_num;

%configuration suppose the center of platform be the origin of world frame
r_platform = 50000; %50000;%platform radius
d_joint_plaform = 57150/2;%17780/2;%distance of adjacent joint on platform
r_base = 80000;%80000;%base radius
d_joint_base = 57150/2;%60000/2;
c = 132500;%70000; %length of linkage
%home_position = 0
base_z=-170000;%z_coordinate of base, dist platform to base

%home position set to center of base
%relative platform position to home position
p = zeros(4,6);
theta1 = asin(d_joint_plaform/r_platform);
theta = theta1;
for i=1:3
  p(1,2*i-1)=cos(theta)*r_platform;
  p(2,2*i-1)=sin(theta)*r_platform;
  p(3,2*i-1)=0;
  p(4,2*i-1)=1;
  theta=theta+2*pi/3; %increments location of vertices, can hard code for paired vertices
end
theta2 = theta1 + 2*pi/3 - 2*theta1;
theta = theta2;
for i=1:3
  p(1,2*i)=cos(theta)*r_platform;
  p(2,2*i)=sin(theta)*r_platform;
  p(3,2*i)=0;
  p(4,2*i)=1;
  theta=theta+2*pi/3; %increments location of vertices, can hard code for paired vertices
end

%relative base position to home position
b = zeros(3,6);
theta1 = pi/3 - asin(d_joint_base/r_base);
theta = theta1;
for i=1:3
  b(1,2*i-1)=cos(theta)*r_base;
  b(2,2*i-1)=sin(theta)*r_base;
  b(3,2*i-1)=base_z;
  theta=theta+2*pi/3; %increments location of vertices, can hard code for paired vertices
end
theta2 = theta1 + 2*asin(d_joint_base/r_base);
theta = theta2;
for i=1:3
  b(1,2*i)=cos(theta)*r_base;
  b(2,2*i)=sin(theta)*r_base;
  b(3,2*i)=base_z;
  theta=theta+2*pi/3; %increments location of vertices, can hard code for paired vertices
end

%get the new leg positions given input
[l_origin,p_origin] = cal_leg_position([0;0;0;0;0;0],p,b,c,base_z);
%base_z = -l_origin(1);
[l_new, new_p] = cal_leg_position(input,p,b,c,base_z);
%l_new;
%set the [0;0;0;0;0;0] input's leg position as 0,platform and base move up
%correspondingly.
%l_origin;
translation=27500;
l_new=l_new-l_origin+translation;
new_p(3,:)=new_p(3,:)-l_origin+translation;
b(3,:)=b(3,:)-l_origin+translation;
l_relative_move = l_new -l_origin+translation;
end

%function cal_leg_position
%Description:
%   Calculate each leg's position given the configuration of platform and
%   the twist of end effector.
%Args:
%   input(6*1 vector): A twist in order of [roll;pitch;yaw;x;y;z]
%
%   p(4*6 matrix): The coordinates of six joints on platform. ith column
%   represents ith joint. Note: For convenience of transformation, add fourth 
%   row and all set to 1.
% 
%   b(3*6 matrix): The coordinates of six joints on base.
%
%   c(1*1 scalar): The length of linkage
%   
%   base_z(1*1 scalar): The z coordinates of base
%Returns:
%   l(1*6 row vector): The z position of six leg.
%
%   new_p(3*6 matrix): The coordinates of six joints on platform.
%   ith column represents ith joint.
function [l_pose,new_p] = cal_leg_position(input,p,b,c,base_z)
%transformation matrix
v = input;
se3mat = VecTose3(v);
T = MatrixExp6(se3mat);
%Get new platform position after transformation
new_p = T * p;
new_p(4,:)=[];

%calculate relative move of each leg
l = 1:6;%leg length respect to base
c_min = 1:6;
angle = 1:6;
for i =1:6
    l(i) = new_p(3,i)-b(3,i)-sqrt(c^2-(new_p(1,i)-b(1,i))^2-(new_p(2,i)-b(2,i))^2);
    c_min(i) = sqrt((new_p(1,i)-b(1,i))^2+(new_p(2,i)-b(2,i))^2);
    angle(i) = acos((new_p(3,i) - (l(i)+base_z))/ c);
end
l_pose=l+base_z;%z coordinate of six legs
%li=l;
c_min_value = max(c_min);
angle_max_value =  max(angle);
end




