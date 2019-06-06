%Script generate_demo_dynamic_matrix
%Description:
%   This script is used for generating demo dynamic_matrix and save it to matrix folder 
%   rather than using self-defined dynamic test in ui.

%dynamic move for roll
w_index=1;
Dynamic_var_A=0.3;
Dynamic_var_Hz=0.2;
[demo_dynamic_matrix,dynamic_matrix_vel,demo_dynamic_matrix_new_p,demo_dynamic_matrix_b,demo_dynamic_matrix_l_new]...
    =generate_dynamic_matrix(w_index,Dynamic_var_A,Dynamic_var_Hz);