% Parameters
N = 50;  % Total number of steps
maxDelay = 15;  % Maximum delay (top speed)
minDelay = 6;  % Minimum delay (bottom speed)
n = 2;  % Acceleration factor (steepness of the curve)
m = 2;  % Deceleration factor (steepness of the curve)

platSteps = [22]; % Plateau durations

% Preallocate a matrix to store delay profiles
allDelays = zeros(length(platSteps), N);
totalSums = zeros(1, length(platSteps));  % To store the sum of each delay profile

% Generate the delay values with plateau in the middle
for h = 1:length(platSteps)
    plateauSteps = platSteps(h);
    accel = (N - plateauSteps) * 0.4; % Acceleration steps
    decel = (N - plateauSteps - accel); % Deceleration steps

    % Compute the delay profile
    delays = zeros(1, N); % Initialize array for current configuration
    for i = 1:N
        if i <= accel
            % Accelerating phase (S-curve)
            t = (i - 1) / accel;  % Normalize time for acceleration
            delays(i) = maxDelay - (maxDelay - minDelay) * (3*t^n - 2*t^3);  % S-curve acceleration
        elseif i > (N - decel)
            % Decelerating phase (reverse S-curve)
            t = (i - (N - decel)) / decel;  % Normalize t for deceleration phase
            delays(i) = minDelay + (maxDelay - minDelay) * (3*t^m - 2*t^3);  % S-curve deceleration
        else
            % Plateau phase at top speed
            delays(i) = minDelay;  % Constant speed
        end
    end

    % Store the delay profile in the matrix
    allDelays(h, :) = delays;

    % Output the delays array for C code
    fprintf('int delayTable90[] = {%d');
    fprintf('%.6g, ', delays(1:end-1)*1000);  % Print all but last element
    fprintf('%.6g};\n', delays(end)*1000);  % Print the last element

    % Calculate the sum of the delay profile
    totalSums(h) = sum(delays);  % Sum of the delays
    fprintf('Sum of delay profile for Plateau = %d: %d\n', plateauSteps, round(totalSums(h)));  % Print sum as integer
end

% Plot all delay profiles
figure;
hold on;
for h = 1:length(platSteps)
    % Use totalSums to include the sum in the legend
    plot(1:N, allDelays(h, :), 'DisplayName', sprintf('Plateau = %d, Sum = %d', platSteps(h), round(totalSums(h))));
end
xlim([0 N]);
ylim([2.5 22.5]);
xlabel('Step');
ylabel('Delay (ms)');
title('S-Curve Profiles with Different Plateau Durations');
legend('show');
grid on;
