%function generate_waypoint_matrix
%Description:
%   Given a .csv file which contains the waypoints(in twist format), t1(time of move 
%   between two adjacent waypoint), t2(time of how long platform stays in that pose 
%   after moving to that waypoint). Generate the six legs's positions and velocities.
%Args:
%   waypoints: waypoints_num*8 matrix for each row, first six value
%   represents twist, 7th value is t1, 8th value is t2. Definition of
%   t1,t2, see description above.
%Returns:
%   waypoints_matrix: waypoints_num*13 matrix, for each row, first 6 values
%   represents six legs' position, 7th to 12th represents the six legs'
%   velocity to move to that waypoint. 13th represents how long platform stays in that pose 
%   after moving to that waypoint.
function waypoints_matrix=generate_waypoint_matrix(waypoints)
    [m,n]=size(waypoints);
    waypoints_matrix=zeros(m,13);
    inputs=[0;0;0;0;0;0]
    [new_p, b, l_new_prev] = merlet_ik(inputs);
    for i =1:m
        inputs=waypoints(i,1:6)';
        [new_p, b, l_new] = merlet_ik(inputs);
        waypoints_matrix(i,1:6)=l_new;
        waypoints_matrix(i,7:12)=(l_new-l_new_prev)/waypoints(i,7);
        waypoints_matrix(i,13)=waypoints(i,8);
    end
end