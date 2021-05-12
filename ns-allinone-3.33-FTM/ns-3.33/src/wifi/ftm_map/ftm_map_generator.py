"""
Copyright (C) 2021 Christos Laskos

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
ERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
"""

import argparse
from scipy import interpolate
import numpy as np
import matplotlib.pyplot as plt
import locale
import scipy.stats as st

default_filename = "FTM_Wireless_Error.map"
default_bias = 10000
default_dcorr = 0.25
default_resolution = 0.01

#heavy_multipath_mean = 0.0 # mean in pico seconds
#heavy_multipath_sd = 5425.71 # standard deviation in pico seconds

def parseArguments():
	parser = argparse.ArgumentParser()
	group1 = parser.add_mutually_exclusive_group()
	group1.add_argument("--xminmax", type=float, nargs=2, metavar=("MIN","MAX"), help="Minimum and Maximum value for X axis. Type: %(type)s")
	group1.add_argument("--x", type=float, help="Creates a symmetrical X axis with [-x/2, x/2].")
	group2 = parser.add_mutually_exclusive_group()
	group2.add_argument("--yminmax", type=float, nargs=2, metavar=("MIN","MAX"), help="Minimum and Maximum value for Y axis.")
	group2.add_argument("--y", type=float, help="Creates a symmetrical Y axis with [-y/2, y/2].")
	parser.add_argument("--dim", type=float, help="Creates both axes with [-dim/2, dim/2].")
	parser.add_argument("--bias", type=int, help="Set the bias for the map, value between [-bias/2, bias/2]. In units of pico seconds.")
	parser.add_argument("--dcorr", type=float, help="Set the decorrelation distance between two points.")
	#parser.add_argument("--resolution", type=float, help="Set the resolution for generated map.")
	parser.add_argument("-o", "--output", type=str, metavar="Filename", help="Set the name for the generated file.")
	group3 = parser.add_mutually_exclusive_group()
	group3.add_argument("--read", action="store_true", help="If set reads from the default map file and visualizes the map.")
	group3.add_argument("--readfile", type=str, metavar="Filename", help="Reads from the specified map file and then visualizes the map.")
	parser.add_argument("--silent", action="store_true", help="Disables the prompt for the file size.")
	parser.add_argument("--heavy_multipath", action="store_true", help="Uses a gaussian distribution parameterized from real world data in a heavy multipath environment to create the map. Bias value is ignored when using this option.")
	return parser.parse_args()

def readFileAndDisplay(filename):
	if filename is None:
		filename=default_filename
	ftm_map = np.loadtxt(filename)
	f = open(filename, "r")
	header = f.readline().rstrip()[2:].split(",")
	f.close()

	xmin = float(header[0].split("=")[1])
	xmax = float(header[1].split("=")[1])
	ymin = float(header[2].split("=")[1])
	ymax = float(header[3].split("=")[1])
	bias = float(header[4].split("=")[1])
	dcorr = float(header[5].split("=")[1])
	resolution = float(header[6].split("=")[1])
	
	x_bins = round((xmax - xmin) / resolution) + 1
	y_bins = round((ymax - ymin) / resolution) + 1

	x_new = np.linspace(xmin, xmax, x_bins)
	y_new = np.linspace(ymin, ymax, y_bins)

	fig, axs = plt.subplots(1)
	
	axs.contourf(x_new, y_new, ftm_map)
	fig.suptitle("FTM Wireless Error Map")
	axs.set_title("x=[" + str(xmin) + "," + str(xmax) + "], y=[" + str(ymin) + "," + str(ymax) + "], bias="+ str(bias) + ", dcorr="+ str(dcorr) + ", resolution=" + str(resolution))
	plt.show()
	

def writeMap(ftm_map, xmin, xmax, ymin, ymax, bias, dcorr, resolution, output):
	if not output.endswith(".map"):
		output += ".map"
	header = "xmin="+str(xmin)+",xmax="+str(xmax)+",ymin="+str(ymin)+",ymax="+str(ymax)+",bias="+str(bias)+",dcorr="+str(dcorr)+",resolution="+str(resolution)+"\n"
	np.savetxt(output, ftm_map, header=header)


def generateMap(args):
	xmin, xmax = 0, 0
	ymin, ymax = 0, 0
	if args.xminmax is not None:
		xmin = args.xminmax[0]
		xmax = args.xminmax[1]
	if args.yminmax is not None:
		ymin = args.yminmax[0]
		ymax = args.yminmax[1]
	if args.x is not None:
		xmin = -args.x / 2
		xmax = args.x / 2
	if args.y is not None:
		ymin = -args.y / 2
		ymax = args.y / 2
	if args.dim is not None:
		xmin = -args.dim / 2
		xmax = args.dim / 2
		ymin = -args.dim / 2
		ymax = args.dim / 2
	if (xmin == 0 and xmax == 0) or (ymin == 0 and ymax == 0):
		print("Please provide dimensions for both axes!\n")
		return
	bias = default_bias
	dcorr = default_dcorr
	resolution = default_resolution
	output = default_filename
	if args.bias is not None:
		bias = args.bias
	if args.dcorr is not None:
		dcorr = args.dcorr
#	if args.resolution is not None:
#		resolution = args.resolution
	if args.output is not None:
		output = args.output
	
	xsize = round((xmax - xmin) / dcorr) + 1
	ysize = round((ymax - ymin) / dcorr) + 1
	x_bins = round((xmax - xmin) / resolution) + 1
	y_bins = round((ymax - ymin) / resolution) + 1

	if not args.silent:
		locale.setlocale(locale.LC_ALL, '') 
		filesize = '{:n}'.format(x_bins * y_bins * 25)
		prompt = "File size will be at least " + filesize + " bytes. Continue? (y/n): "
		answer = str(input(prompt)).lower().strip()
		if answer[:1] != 'y':
			return


	x = np.linspace(xmin, xmax, xsize)
	y = np.linspace(ymin, ymax, ysize)
	xx, yy = np.meshgrid(x, y)

	z = None
	if args.heavy_multipath:
		#z = np.random.normal(loc=heavy_multipath_mean, scale=heavy_multipath_sd, size=xx.shape)
		z = st.exponnorm.rvs(1.9422496573694217, -1.6435585024441102, 0.8462059922427465, size=xx.shape) * 100 / 0.03
	else:
		z = np.random.rand(xx.shape[0], xx.shape[1]) * bias - bias/2

	f = interpolate.interp2d(x, y, z, kind='cubic')

	x_new = np.linspace(xmin, xmax, x_bins)
	y_new = np.linspace(ymin, ymax, y_bins)

	ftm_map = f(x_new, y_new)

	writeMap(ftm_map, xmin, xmax, ymin, ymax, bias, dcorr, resolution, output)


def main():
	args = parseArguments()

	if args.read or args.readfile:
		readFileAndDisplay(args.readfile)
		return
	generateMap(args)


if __name__ == "__main__":
	main()

