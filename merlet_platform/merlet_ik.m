function [new_p, b, l_new,l_relative_move] = merlet_ik (input)
addpath('~/Desktop/EECS495_Robot_Studio/merlet/mr')
% input = [0;-pi/9;0;0;0;0];%input=(roll;pitch,yaw;x;y;z)
%print("the input is:",input);
%configuration suppose the center of platform be the origin of world frame
fraction = 0.5;
r_platform = 77470;%platform radius
d_joint_plaform = 17780/2;%distance of adjacent joint on platform
r_base = 142240;%base radius
d_joint_base = 58000/2;
c = 130000; %length of linkage
base_z=-250000;%z_coordinate of base, dist platform to base

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
% for i=1:6
%   b(1,i)=cos(theta)*r_base;
%   b(2,i)=sin(theta)*r_base;
%   b(3,i)=base_z;
%   theta=theta+pi/3; 
% end

[l_origin,p_origin] = cal_li([0;0;0;0;0;0],p,b,c,base_z)
[l_new, new_p] = cal_li(input,p,b,c,base_z);
l_relative_move =  l_new - l_origin

%plot
%platform

% p_plot=[new_p new_p(:,1)]
% plot3(p_plot(1,:),p_plot(2,:),p_plot(3,:));
% hold on
% %joint
% joint_pos = b;
% joint_pos(3,:)=l_new;
% joint_plot = [joint_pos joint_pos(:,1)];
% plot3(joint_plot(1,:),joint_plot(2,:),joint_plot(3,:));
% hold on
% %base
% b_plot=[b b(:,1)];
% plot3(b_plot(1,:),b_plot(2,:),b_plot(3,:));
% hold on
% %link three hexagon
% for i=1:6
%     link_plot = [new_p(:,i) joint_pos(:,i) b(:,i)];
%     plot3(link_plot(1,:),link_plot(2,:),link_plot(3,:));
%     hold on
% end
% hold off
end


function [li,new_p] = cal_li(input,p,b,c,base_z)%calculate each leg's position
%transformation matrix
v = input;
se3mat = VecTose3(v);
T = MatrixExp6(se3mat);
%new platform position after transformation
%global new_p
new_p = T * p;
new_p(4,:)=[];

%calculate relative move of each leg
l = 1:6;
c_min = 1:6;
angle = 1:6;
x = [1,0,0;0,1,0;0,0,1];%unit vector
for i =1:6
    l(i) = new_p(3,i)-b(3,i)-sqrt(c^2-(new_p(1,i)-b(1,i))^2-(new_p(2,i)-b(2,i))^2);
    c_min(i) = sqrt((new_p(1,i)-b(1,i))^2+(new_p(2,i)-b(2,i))^2);
    angle(i) = acos((new_p(3,i) - (l(i)+base_z))/ c);
end
li=l+base_z;
c_min_value = max(c_min)
angle_max_value =  max(angle)
end


