import pathlib
import subprocess
import time
import glob
import csv
import numpy as np
from scipy.optimize import minimize
import seaborn as sns
import pandas as pd
import matplotlib.pyplot as plt
import os

map_path = "ftm_lateration/ftm.map"
results_path = "ftm_lateration/results/"
outlier_threshold = 2.0

def createDir():
	pathlib.Path('ftm_lateration/').mkdir(parents=False, exist_ok=True)
	for f in glob.glob("ftm_lateration/*.csv"):
		os.remove(f) #remove old csv files
	for f in glob.glob("ftm_lateration/*.png"):
		os.remove(f) #remove old png files
#	global map_path
#	map_path = "ftm_lateration/ftm.map"

def createResultsDir():
	pathlib.Path(results_path).mkdir(parents=False, exist_ok=True)

def performTests():
	for i in range(0,100):
		print("Run: " + str(i + 1))
		subprocess.run(["python", "src/wifi/ftm_map/ftm_map_generator.py", "--dim=20", "--heavy_multipath", "--silent", "-o", map_path])

		#varying FTMs
		positions = 10
		seed = i + 1
		ftms = [2, 4, 6, 8, 10, 20]
		bursts = 0
		error_model = "wired"
		bandwidth = 20

		for ftm in ftms:
			waf_command = ["--run=ftm-lateration", "--positions=" + str(positions), "--seed=" + str(seed), "--ftms=" + str(ftm),
				   		   "--bursts=" + str(bursts), "--errorModel=" + error_model, "--bandwidth=" + str(bandwidth)]
			waf_string = ' '.join(waf_command)
			#20MHz wired error
			#subprocess.run(["./waf", waf_string])

			waf_command[6] = "--bandwidth=40"
			waf_string = ' '.join(waf_command)
			#40MHz wired error
			#subprocess.run(["./waf", waf_string])

			waf_command[5] = "--errorModel=wireless"
			#append map for wireless error
			waf_command.append("--map=" + map_path)
			waf_command[6] = "--bandwidth=20"
			waf_string = ' '.join(waf_command)
			#20MHz wireless error
			subprocess.run(["./waf", waf_string])

			waf_command[6] = "--bandwidth=40"
			waf_string = ' '.join(waf_command)
			#40MHz wireless error
			subprocess.run(["./waf", waf_string])

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

def generateOutlierGraphs(outliers):
	for f in glob.glob("ftm_lateration/*.png"):
		os.remove(f) #remove old png files
	for outlier in outliers:
		file_name = outlier[0][:-4]
		data = outlier[1]
		plt.clf()
		fig, axs = plt.subplots(1, len(data), figsize=(16,9))
		j = 0
		for d in data:
			ax = None
			if len(data) == 1:
				ax = axs
			else:
				ax = axs[j]
			xy = d[0]
			distances = d[1][0]
			stations = d[1][1]
			ax.set_title("Distance: " + str(round(np.linalg.norm(xy), 2)))
			ax.plot(xy[0], xy[1], 'x', color="r", label="Calculated Position")
			ax.legend()
			ax.axis('scaled')
			ax.set_xlabel("Meters")
			ax.set_ylabel("Meters")
			ax.set_xlim((-15, 15))
			ax.set_ylim((-15, 15))
			
			for i in range(0, len(distances)):
				circle = plt.Circle((stations[i][0], stations[i][1]), distances[i], color='g', fill=False, linewidth=0.5)
				ax.plot(stations[i][0], stations[i][1], 'x', color="b")
				ax.add_patch(circle)
			j += 1
			#print(distances)
		fig.savefig(file_name)
		plt.close()
	return

def calculateResults():
	wired_file = open(results_path + "wired.csv", 'w', newline='')
	wireless_file = open(results_path + "wireless.csv", 'w', newline='')
	wired_csv = csv.writer(wired_file, delimiter=';')
	wireless_csv = csv.writer(wireless_file, delimiter=';')
	wired_csv.writerow(["bandwidth", "positions", "euclidean_error", "ftms", "seed", "x_calculated", "y_calculated"])
	wireless_csv.writerow(["bandwidth", "positions", "euclidean_error", "ftms", "seed", "x_calculated", "y_calculated"])

	outliers = list()

	files = sorted([f for f in glob.glob("ftm_lateration/*.csv")])
	for f in files:
		writer = None
		attributes = f.split('/')[1].split('_')
		if attributes[0] == "wired":
			writer = wired_csv
		elif attributes[0] == "wireless":
			writer = wireless_csv
		else:
			continue

		stations = list()
		distances = list()

		csv_reader = csv.reader(open(f), delimiter=';')
		line = 0
		for row in csv_reader:
			if line == 0:
				line += 1
				continue
			stations.append(np.array([float(row[1]), float(row[2])]))
			distances.append(float(row[0]) * 0.03 / 2 / 100)
			line += 1

		current_outliers = list()

		xy = gps_solve(distances, stations)
		if round(np.linalg.norm(xy), 2) > outlier_threshold:
			current_outliers.append([xy, [distances, stations]])
			#continue
		output = [attributes[1][:-3], str(len(stations)), str(round(np.linalg.norm(xy), 2)), attributes[2][:-4], attributes[3].split('.')[0][:-4],
				 str(np.round(xy[0], 5)), str(np.round(xy[1], 5))]
		writer.writerow(output)

		if len(current_outliers) > 0:
			outliers.append([f, current_outliers])

	wired_file.close()
	wireless_file.close()
	#generateOutlierGraphs(outliers)

def visualize():
	sns.set(style="ticks", palette="dark")
	"""
	wired = pd.read_csv(results_path + "wired.csv", sep=';')
	ax = sns.boxplot(x="ftms", y="euclidean_error",
            hue="bandwidth", palette=["magenta", "green"],
            data=wired)
	sns.despine(offset=0, trim=True)
	ax.set_ylim([0, outlier_threshold])
	ax.yaxis.set_ticks(np.linspace(0, outlier_threshold, round(outlier_threshold / 0.1) + 1))
	ax.legend(loc=1, bbox_to_anchor=(1, 1.08), frameon=True, framealpha=1)
	plt.xlabel("FTMs")
	plt.ylabel("Euclidean Error")
	plt.title("Wired\n")#Outlier Threshold: >" + str(outlier_threshold))
	plt.savefig(results_path + "wired.png")
	plt.clf()
	plt.close()
	"""
	wireless = pd.read_csv(results_path + "wireless.csv", sep=';')
	ax = sns.boxplot(x="ftms", y="euclidean_error",
            hue="bandwidth", palette=["magenta", "green"],
            data=wireless)
	sns.despine(offset=0, trim=True)
	ax.set_ylim([0, outlier_threshold])
	ax.yaxis.set_ticks(np.linspace(0, outlier_threshold, round(outlier_threshold / 0.25) + 1))
	ax.legend(loc=1, bbox_to_anchor=(1, 1.08), frameon=True, framealpha=1)
	plt.xlabel("FTMs")
	plt.ylabel("Euclidean Error")
	plt.title("Wireless\n")#Outlier Threshold: >" + str(outlier_threshold))
	plt.savefig(results_path + "wireless.png")
	plt.clf()
	plt.close()

def main():
	createDir()
	performTests()
	createResultsDir()
	calculateResults()
	visualize()

if __name__ == "__main__":
	start_time = time.perf_counter()
	main()
	end_time = time.perf_counter()
	time_elapsed = round(end_time - start_time, 3)
	print("DONE")
	print("Time elapsed: " + str(time_elapsed) + 's')

