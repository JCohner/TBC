%Script maxium_range_of_leg
%Description:
%   This script set all 32 extreme pose of platform. Except parameter z,
%   there are 5 parameters with range [-0.34,0.34] (20 degrees in radius)
%   or [-1,1]. SO there is 2^5=32 extreme poses. Find the maxium and minimum
%   leg length among these 32*6 values. The take parameter z into
%   consideration, the range should be [minimum-1,maxium+1]  
extreme_test = zeros(32,5);
extreme_test(1,:) = [1,1,0.34,0.34,0.34];
extreme_test(2,:) = [1,1,0.34,0.34,-0.34];
extreme_test(3,:) = [1,1,0.34,-0.34,0.34];
extreme_test(4,:) = [1,1,0.34,-0.34,-0.34];
extreme_test(5,:) = [1,1,-0.34,0.34,0.34];
extreme_test(6,:) = [1,1,-0.34,0.34,-0.34];
extreme_test(7,:) = [1,1,-0.34,-0.34,0.34];
extreme_test(8,:) = [1,1,-0.34,-0.34,-0.34];
extreme_test(9,:) = [1,-1,0.34,0.34,0.34];
extreme_test(10,:) = [1,-1,0.34,0.34,-0.34];
extreme_test(11,:) = [1,-1,0.34,-0.34,0.34];
extreme_test(12,:) = [1,-1,0.34,-0.34,-0.34];
extreme_test(13,:) = [1,-1,-0.34,0.34,0.34];
extreme_test(14,:) = [1,-1,-0.34,0.34,-0.34];
extreme_test(15,:) = [1,-1,-0.34,-0.34,0.34];
extreme_test(16,:) = [1,-1,-0.34,-0.34,-0.34];

extreme_test(17,:) = [-1,1,0.34,0.34,0.34];
extreme_test(18,:) = [-1,1,0.34,0.34,-0.34];
extreme_test(19,:) = [-1,1,0.34,-0.34,0.34];
extreme_test(20,:) = [-1,1,0.34,-0.34,-0.34];
extreme_test(21,:) = [-1,1,-0.34,0.34,0.34];
extreme_test(22,:) = [-1,1,-0.34,0.34,-0.34];
extreme_test(23,:) = [-1,1,-0.34,-0.34,0.34];
extreme_test(24,:) = [-1,1,-0.34,-0.34,-0.34];
extreme_test(25,:) = [-1,-1,0.34,0.34,0.34];
extreme_test(26,:) = [-1,-1,0.34,0.34,-0.34];
extreme_test(27,:) = [-1,-1,0.34,-0.34,0.34];
extreme_test(28,:) = [-1,-1,0.34,-0.34,-0.34];
extreme_test(29,:) = [-1,-1,-0.34,0.34,0.34];
extreme_test(30,:) = [-1,-1,-0.34,0.34,-0.34];
extreme_test(31,:) = [-1,-1,-0.34,-0.34,0.34];
extreme_test(32,:) = [-1,-1,-0.34,-0.34,-0.34];

z_column = zeros(32,1);
extreme_test = [extreme_test(:,1:2) z_column extreme_test(:,3:5)];
extreme_test = extreme_test';
extreme_test = [extreme_test(4:6,:); extreme_test(1:3,:)];
extreme_test(extreme_test==0.34)=0.35;
extreme_test(extreme_test==-0.34)=-0.35;
%extreme_test = extreme_test*0.8;
l_new_max =1.5*1e4;
l_new_min =1.5*1e4;
for i = 1:32
    test_input = extreme_test(:,i);
    [new_p, b, l_new,l_relative_move] = merlet_ik(test_input);
    l_new_max_curr = max(l_new);
    l_new_min_curr = min(l_new);
    if l_new_max_curr>l_new_max
        l_new_max = l_new_max_curr;
        l_new_max_i = i;
    end
    if l_new_min_curr<l_new_min
        l_new_min = l_new_min_curr;
        l_new_min_i = i;
    end
end
l_new_max
l_new_min
l_new_max-l_new_min

