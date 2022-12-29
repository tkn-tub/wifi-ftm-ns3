import numpy as np
import glob
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns
from scipy.optimize import minimize
import scipy.stats as st

# source: https://github.com/glucee/Multilateration
def gps_solve(distances_to_station, stations_coordinates):
	def error(x, c, r):
		return sum([(np.linalg.norm(x - c[i]) - r[i]) ** 2 for i in range(len(c))])

	l = len(stations_coordinates)
	S = sum(distances_to_station)
	# compute weight vector for initial guess
	W = [((l - 1) * S) / (S - w) for w in distances_to_station]
	# get initial guess of point location
	x0 = sum([W[i] * stations_coordinates[i] for i in range(l)])
	# optimize distance from signal origin to border of spheres
	return minimize(error, x0, args=(stations_coordinates, distances_to_station), method='Nelder-Mead').x

def rtt_to_meters(rtt):
    return np.round(rtt / 2 / 1000 * 0.3, 2) # also round to cm accuracy

def findClosest(num,collection):
   return min(collection,key=lambda x:abs(x-num))

def combine_measurements(rtt, rssi, error_model, correction_values):
    if error_model == "sig_str_fading_rssi_aware":
        #run for each RTT individually and mean afterwards
        # for i in range(len(rtt)):
        #     curr_rssi = rssi[i]
        #     closest_sig_str = findClosest(curr_rssi, list(correction_values.keys()))
        #     new_rrt = rtt[i] - correction_values[closest_sig_str]
        #     rtt[i] = new_rrt
        # mean = np.mean(rtt)
        # return rtt_to_meters(mean)
    
        # #take mean first and then correct
        # mean_rtt = np.mean(rtt)
        # mean_rssi = np.mean(rssi)
        # closest_sig_str = findClosest(mean_rssi, list(correction_values.keys()))
        # mean_rtt -= correction_values[closest_sig_str]
        # return rtt_to_meters(mean_rtt)
        
        #weigh the measurements based on the rssi they have
        if len(rtt) == 1:
            return rtt_to_meters(np.mean(rtt))
        rtt_new = np.array(rtt)
        rssi_new = np.array(rssi)
        rssi_new = 10**(rssi_new/10) #convert to linear
        rssi_sum = sum(rssi_new)
        w = [x/rssi_sum for x in rssi_new] #calculate weight based on significance in total rssi
        for i in range(len(w)):
            rtt_new[i] = rtt_new[i] * w[i] #multiply by the weights
        rtt_guess = sum(rtt_new) #guessed RTT is the sum of the weighted RTTs
        return rtt_to_meters(rtt_guess)
    else:
        mean = np.mean(rtt)
        return rtt_to_meters(mean)
    
def load_johnsonsu_mean_values():
    dists = np.loadtxt("johnsonsu")
    mean_vals = {}
    for i in range(len(dists)):
        curr_dist = dists[i]
        values = st.johnsonsu.rvs(curr_dist[2], curr_dist[3], curr_dist[4], curr_dist[5], size=2500)
        mean = np.mean(values)
        key = int(curr_dist[0])
        mean_vals[key] = mean
    return mean_vals

johnson_correction_values = load_johnsonsu_mean_values()

error_models = ["wired", "wireless", "sig_str", "sig_str_fading", "sig_str_fading_rssi_aware"]

for curr_error_model in error_models:
    files = sorted(glob.glob(curr_error_model + "/*"))
    ftm_measurements = [1,2,4,8,16,39]
    num_positions = 10
    #each 10 rows are the measurements from one run, colums are the ftm_measurements combined and transformed to distance
    data = np.zeros((len(files) * num_positions, len(ftm_measurements))) 
    positions_sta = np.zeros((len(files) * num_positions, 2))
    data_index = 0
    for f in files:
        curr_measurement = np.loadtxt(f)
        for j in range(10):
            curr_position = curr_measurement[j*39:(j+1)*39,:]
            positions_sta[data_index,0] = curr_position[0,2]
            positions_sta[data_index,1] = curr_position[0,3]
            
            for k in range(len(ftm_measurements)):
                tmp_rtt = curr_position[0:ftm_measurements[k],0]
                tmp_rssi = curr_position[0:ftm_measurements[k],1]
                measured_distance = combine_measurements(tmp_rtt, tmp_rssi, curr_error_model, johnson_correction_values)
                data[data_index, k] = measured_distance
            
            data_index += 1
            
    
    
    localization_errors = []
    
    for i in range(0, len(data), 10): # go through all the data
        measurement_distances = data[i:i+10] # select one measurement each time, 10 positions
        measurement_positions = positions_sta[i:i+10]
        for j in range(3, len(measurement_distances)+1): # increase positions from 3 to 10
            used_measurements = measurement_distances[0:j]
            used_positions = measurement_positions[0:j]
            for k in range(len(ftm_measurements)): # increase used ftm measurements
                tmp_ftms = used_measurements[:,k]
                calculated_pos = gps_solve(tmp_ftms, used_positions)
                position_error = np.round(np.linalg.norm(calculated_pos), 2)
                localization_errors.append([position_error, ftm_measurements[k], j])
        
    df = pd.DataFrame(data=localization_errors, columns=["location_error", "ftms", "positions"])
    
    #height=2.5, aspect=12/8
    fig = plt.figure(figsize=(2.5*12/8,2.5))
    plt.rcParams['font.size'] = '7'
    p = sns.boxplot(data=df, x='positions', y='location_error', hue='ftms', fliersize=0.5, linewidth=0.5,
                showfliers=False)
    p.legend_.set_title("FTMs F")
    plt.xlabel("Positions P")
    plt.ylabel("Euclidean error [m]")
    plt.yticks(np.arange(0,5.5,0.5))
    plt.ylim(0, 5)
    plt.grid(linewidth=0.5, alpha=0.35, linestyle='--')
    plt.tight_layout()
    plt.savefig(curr_error_model + ".pdf")
    # break
