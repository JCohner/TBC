%Script generate_demo_dynamic_matrix
%Description:
%   This script is used for generating demo dynamic_matrix and save it to matrix folder 
%   rather than using self-defined dynamic test in ui.

%dynamic move for roll
w_index=1;
Dynamic_var_A=0.3;
Dynamic_var_Hz=1;
[demo2_dynamic_matrix,dynamic_matrix_vel,demo_dynamic_matrix_new_p,demo_dynamic_matrix_b,demo_dynamic_matrix_l_new]...
    =generate_dynamic_matrix(w_index,Dynamic_var_A,Dynamic_var_Hz);
% waypoints_num=100;
% plot_dynamic_matrix(demo_dynamic_matrix_new_p,...
%    demo_dynamic_matrix_b,demo_dynamic_matrix_l_new,waypoints_num) 

% for plot, but not used anymore
function plot_merlet(new_p,l_new,b)
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

function results = plot_dynamic_matrix(dynamic_matrix_new_p,...
    dynamic_matrix_b,dynamic_matrix_l_new,waypoints_num)          
    for t=1:waypoints_num
        %plot3(app.UIAxes,1,1,1);
        new_p=dynamic_matrix_new_p(:,:,t);
        b=dynamic_matrix_b(:,:,t);
        l_new=dynamic_matrix_l_new(:,t);
        plot_merlet(new_p, b, l_new);
        %fprintf('here we are %6.2f.\n',t);
        pause(0.05);
    end
end