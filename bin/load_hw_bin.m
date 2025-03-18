function load_hw_bin(filename)
fp = fopen(filename, 'rb');
x = fread(fp, Inf, 'float64');
fclose(fp);
qqplot(x);
x_mean = mean(x);
x_std = std(x);
fprintf('Number of points: %d\n', numel(x));
fprintf('mean = %g, std = %g\n', x_mean, x_std);
[h, p, kstat, critval] = lillietest(x)
end
