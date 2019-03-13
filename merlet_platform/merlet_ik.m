addpath('~/Desktop/EECS495_Robot_Studio/mr')
input = [0;-pi/9;0;0;0;0];%input=(roll;pitch,yaw;x;y;z)

%configuration suppose the center of platform be the origin of world frame
r_platform = 7;%platform radius
r_base = 10;%base radius
c = 21; %length of linkage
base_z=-12;%z_coordinate of base

%relative platform position to home position
p = zeros(4,6);
theta = 0;
for i=1:6
  p(1,i)=cos(theta)*r_platform;
  p(2,i)=sin(theta)*r_platform;
  p(3,i)=0;
  p(4,i)=1;
  theta=theta+pi/3;
end


%relative base position to home position
b = zeros(3,6)
theta = 0
for i=1:6
  b(1,i)=cos(theta)*r_base;
  b(2,i)=sin(theta)*r_base;
  b(3,i)=base_z;
  theta=theta+pi/3;
end

l_origin = cal_li([0;0;0;0;0;0],p,b,c);
l_new = cal_li(input,p,b,c);
l_relative_move =  l_new - l_origin

%plot
%platform
global new_p
p_plot=[new_p new_p(:,1)]
plot3(p_plot(1,:),p_plot(2,:),p_plot(3,:));
hold on
%joint
joint_pos = b;
joint_pos(3,:)=l_new;
joint_plot = [joint_pos joint_pos(:,1)];
plot3(joint_plot(1,:),joint_plot(2,:),joint_plot(3,:));
hold on
%base
b_plot=[b b(:,1)];
plot3(b_plot(1,:),b_plot(2,:),b_plot(3,:));
hold on
%link three hexagon
for i=1:6
    link_plot = [new_p(:,i) joint_pos(:,i) b(:,i)];
    plot3(link_plot(1,:),link_plot(2,:),link_plot(3,:));
    hold on
end

function li = cal_li(input,p,b,c)%calculate each leg's position
%transformation matrix
v = input;%v=(roll;pitch,yaw;x;y;z)
se3mat = VecTose3(v);
T = MatrixExp6(se3mat)
%new platform position after transformation
global new_p
new_p = T * p;
new_p(4,:)=[]

%calculate relative move of each leg
l = 1:6;
x = [1,0,0;0,1,0;0,0,1];%unit vector
for i =1:6
    l(i) = new_p(3,i)-b(3,i)-sqrt(c^2-(new_p(1,i)-b(1,i))^2-(new_p(2,i)-b(2,i))^2)
end
li=l;
end


