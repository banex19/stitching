import os
import sys
import time
import subprocess
import re
from shutil import copy2

def getTime():
	return time.perf_counter()

def writeTimes(times, filename):
	times.sort()
	times = times[1:-1]
		
	file = open(filename,"w") 
	for time in times:
		file.write(str(time))
		file.write("\n")	
	file.close()

NUM_TESTS =  23
NUM_DISCARD = 1

SIMPLE_PROGRAM =  'siege -t15S '
 
STITCHED_PROGRAM = 'siege -t15S '

SIMPLE_PROGRAM_ARGS =  " http://127.0.0.1:10"
STITCHED_PROGRAM_ARGS = " http://127.0.0.1:80"

doSimple = True
onlySimple = False
nothing = False

if len(sys.argv) > 1:
	if sys.argv[1] == "nosimple" or sys.argv[1] == "onlysimple" or sys.argv[1] == "nothing":
		argNumber = 1
	else:
		argNumber = 2
		STITCHED_PROGRAM_ARGS = sys.argv[1]
		print("Using stitched program args: " + STITCHED_PROGRAM_ARGS)

	if len(sys.argv) > argNumber:
		if sys.argv[argNumber] == "nosimple":
			doSimple = False
		if sys.argv[argNumber] == "onlysimple":
			onlySimple = True
		if sys.argv[argNumber] == "nothing":
			nothing = True

if not nothing:
	if doSimple:
		times = []

		for i in range(0, NUM_TESTS):

			output = subprocess.check_output(SIMPLE_PROGRAM + SIMPLE_PROGRAM_ARGS, stderr=subprocess.STDOUT, shell=True)

			transactions = re.search('Transaction rate:(.*?)([0-9\.]+).*', output.decode('utf-8'))

			result = transactions.group(2);

			print ("Run (simple) " + str(i) + " completed - " + result);

			if i >= NUM_DISCARD:
				times.append(result)

			time.sleep(0.5);	

		writeTimes(times, "results_apache_simple.txt")

	if not onlySimple:
		times = []

		for i in range(0, NUM_TESTS):
			output = subprocess.check_output(STITCHED_PROGRAM + STITCHED_PROGRAM_ARGS, stderr=subprocess.STDOUT, shell=True)

			transactions = re.search('Transaction rate:(.*?)([0-9\.]+).*', output.decode('utf-8'))

			result = transactions.group(2);

			print ("Run (stitched) " + str(i) + " completed - " + result);

			if i >= NUM_DISCARD:
				times.append(result)

			time.sleep(0.5);	

		writeTimes(times, "results_apache_stitched.txt")

currTime = time.strftime("%y_%m_%d_%H_%M_%S")

copy2("results_apache_simple.txt", "logs_apache/results_apache_simple_" + currTime + ".txt")
copy2("results_apache_stitched.txt", "logs_apache/results_apache_stitched_" + currTime + ".txt")

file = open("logs_apache/results_apache_info_" + currTime + ".txt","w") 
file.write("SIMPLE PROGRAM = " + SIMPLE_PROGRAM);
file.write("\n");		
file.write("STITCHED PROGRAM = " + STITCHED_PROGRAM);
file.write("\n");	
file.write("SIMPLE PROGRAM ARGS = " + SIMPLE_PROGRAM_ARGS);
file.write("\n");	
file.write("STITCHED PROGRAM ARGS = " + STITCHED_PROGRAM_ARGS);
file.write("\n");	
file.close()


os.system("ministat results_apache_simple.txt results_apache_stitched.txt")


#tramp  150
#cb     90
#fixed  80
#simple 10