import numpy as np
from scipy.optimize import minimize, minimize_scalar
# from skopt import gp_minimize
import matplotlib.pyplot as plt

# Define a mock 2D Gaussian function representing the lightshift with noise
def gaussian_2d(x, x0, y0, sigma_x, sigma_y):
    """Returns the value of a 2D Gaussian function."""
    return np.exp(-(((x[0] - x0) ** 2) / (2 * sigma_x ** 2) + ((x[1] - y0) ** 2) / (2 * sigma_y ** 2)))

# Noise handling - returns the value of lightshift with added Gaussian noise
def measure_lightshift_with_noise(beam_position, true_position=(5, 5), noise_level=0.05):
    true_value = gaussian_2d(beam_position, *true_position, sigma_x=2, sigma_y=2)
    noise = np.random.normal(0, noise_level)
    return true_value + 0*noise

# Count the number of times the lightshift function is called
class LightshiftCounter:
    def __init__(self):
        self.counter = 0

    def measure(self, beam_position):
        self.counter += 1
        return measure_lightshift_with_noise(beam_position)

# BFGS Optimization with stochastic noise
def optimize_lightshift_bfgs_with_simulation(initial_beam_position, max_iterations=50, method='BFGS'):
    counter = LightshiftCounter()

    def objective_function(beam_position):
        return -counter.measure(beam_position)  # Minimizing negative lightshift

    result = minimize(
        objective_function,
        np.array(initial_beam_position),
        method=method,
        options={
            'disp': False,  # Disable messages
            'maxiter': max_iterations,
        }
    )
    return counter.counter, result.x

# Gaussian Process Optimization with stochastic noise
def optimize_lightshift_gp_with_simulation(initial_beam_position, max_iterations=50):
    counter = LightshiftCounter()

    def objective_function(beam_position):
        return -counter.measure(beam_position)  # Minimizing negative lightshift

    bounds = [(initial_beam_position[0] - 10, initial_beam_position[0] + 10), 
              (initial_beam_position[1] - 10, initial_beam_position[1] + 10)]

    result = gp_minimize(objective_function, dimensions=bounds, n_calls=max_iterations, verbose=False)
    
    return counter.counter, result.x

# Initial beam position and settings
initial_position = (0, 0)
max_iterations = 50

# Run both optimization methods
bfgs_calls, bfgs_result = optimize_lightshift_bfgs_with_simulation(initial_position, max_iterations, method='BFGS')
# gp_calls, gp_result = optimize_lightshift_gp_with_simulation(initial_position, max_iterations)

# Display results
print(bfgs_calls, bfgs_result)


# Global variables to track evaluations
x_evaluations = 0
y_evaluations = 0

# 1D Gaussian function for a single axis
def gaussian_1d(x, x0, sigma):
    return np.exp(-((x - x0) ** 2) / (2 * sigma ** 2))

# Mock lightshift function along a single axis with noise and evaluation count
def measure_lightshift_1d(x, true_position=5, sigma=2, noise_level=0.05, axis='x'):
    global x_evaluations, y_evaluations
    
    if axis == 'x':
        x_evaluations += 1
    elif axis == 'y':
        y_evaluations += 1
    
    true_value = gaussian_1d(x, true_position, sigma)
    noise = np.random.normal(0, noise_level)
    return true_value + noise

# Optimize a single axis using 1D optimizer
def optimize_single_axis(true_position=5, initial_position=0, axis='x'):
    result = minimize_scalar(
        lambda x: -measure_lightshift_1d(x, true_position=true_position, axis=axis),  # Maximizing lightshift
        bounds=(-10, 10),
        method='bounded',
    )
    return result

# 2D Optimization by alternating between x and y axes
def coordinate_descent_2d(true_position=(5, 5), initial_position=(0, 0), max_iters=10):
    global x_evaluations, y_evaluations
    x_evaluations, y_evaluations = 0, 0  # Reset evaluation counters
    
    x0, y0 = initial_position
    for i in range(max_iters):
        # Optimize x axis
        result_x = optimize_single_axis(true_position=true_position[0], initial_position=x0, axis='x')
        x0 = result_x.x
        
        # Optimize y axis
        result_y = optimize_single_axis(true_position=true_position[1], initial_position=y0, axis='y')
        y0 = result_y.x
        
        print(f"Iteration {i+1}: x = {x0}, y = {y0}, x evals = {x_evaluations}, y evals = {y_evaluations}")
    
    return (x0, y0)

# Run the coordinate descent
final_position = coordinate_descent_2d()
print("Final position:", final_position)
print(f"Total x evaluations: {x_evaluations}, Total y evaluations: {y_evaluations}")