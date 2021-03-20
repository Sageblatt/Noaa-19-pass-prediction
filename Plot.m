a = csvread("out.csv");
figure(1);
polarplot(a(:, 1), a(:, 2));
n = csvread("outc.csv");
figure(2);
geoscatter(n(:, 1), n(:, 2), 2, "b");