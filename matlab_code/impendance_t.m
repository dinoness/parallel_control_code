f_sensor = -1.9;
f_ref = 5;
f_ref0 = 0 : 1000 : 5;
dz_sum = 0;
dz = 0;
f_err = f_sensor - f_ref;
e2 = 0;
e1 = 0;
e0 = 0;
m = 2.5;
b = 160;
k = 1000;
T = 0.001;
ke = 1000;

dz_list = zeros(3000,1);
dz_sum_list = zeros(3000,1);
f_ref_list = zeros(3000,1) + 5;
f_ref_list(1:length(f_ref0)) = f_ref0; 
n_length = length(dz_list);
for i = 1 : n_length
    f_err = f_sensor - f_ref_list(i) - ke * e0;
    e0 = (T^2 * f_err + (T*b + 2*m) * e1 - m * e2) / (m + T*b + T^2 * k);
    if(abs(e0-e1) > 1e-5)
        e0 = e1 + 1e-5 * sign(e0-e1);
    end

    dz = e0*1000000;
    if abs(dz) > 5000
        dz = dz/abs(dz)*5000;
    end

    e2 = e1;
    e1 = e0;
    dz_list(i) = dz;

end
figure(1)
plot(dz_list)

% figure(2)
% plot(dz_sum_list)