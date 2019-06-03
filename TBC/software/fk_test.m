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
initial_twist  = inputs;
%[new_p, b, l_new] = merlet_ik(inputs);
% for i=1:100
%     i
%     initial_twist    
%     [twist,new_p, b, l_new,l_relative_move,iter_num]=merlet_fk(demo2_dynamic_matrix(i,:),initial_twist);
%     twist
%     iter_num
%     fk_twist(:,i)=twist;
%     initial_twist=twist;
% end

%leg1 increase 
% leg1_1000_waypoints=zeros(1000,6);
% for i=1:1000
%     increase = i*2;
%     leg1_1000_waypoints(i,:)=[increase,0,0,0,0,0]+15000;
% end
for i=1:1000
    i
    initial_twist    
    [twist,new_p, b, l_new,l_relative_move,iter_num]=merlet_fk(leg1_1000_waypoints(i,:),initial_twist);
    twist
    iter_num
    fk_twist(:,i)=twist;
    initial_twist=twist;
end

%plot the predict twist
% for i=1:100
%     [new_p, b, l_new,l_relative_move] = merlet_ik (fk_twist(:,i));
%     merlet_plot(new_p,l_new,b);
% end

function merlet_plot(new_p,l_new,b)
%plot
%platform

p_plot=[new_p new_p(:,1)];
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
hold off
end