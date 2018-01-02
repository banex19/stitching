import os
import sys
import time
import subprocess
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

SIMPLE_PROGRAM =  "/home/daniele/test/libs/kann_simple/examples/rnn-bit"	# "./simple" # "/home/daniele/test/libs/mprime/chudnovsky-demo/pi" # 
 
STITCHED_PROGRAM =  "/home/daniele/test/libs/kann_cb/examples/rnn-bit" # "./stitched_cb"  # #  "/home/daniele/test/libs/mprime/chudnovsky-demo/stitched" #

SIMPLE_PROGRAM_ARGS = " -m5 -o add.kan /home/daniele/test/libs/kann/examples/train.txt" # "" # " 2 2000 50000 " #
STITCHED_PROGRAM_ARGS = SIMPLE_PROGRAM_ARGS

doSimple = True
onlySimple = False
nothing = False

if len(sys.argv) > 1:
	if sys.argv[1] == "nosimple" or sys.argv[1] == "onlysimple" or sys.argv[1] == "nothing":
		argNumber = 1
	else:
		argNumber = 2
		STITCHED_PROGRAM = sys.argv[1]
		print("Using stitched program: " + STITCHED_PROGRAM)

	if len(sys.argv) > argNumber:
		if sys.argv[argNumber] == "nosimple":
			doSimple = False
		if sys.argv[argNumber] == "onlysimple":
			onlySimple = True
		if sys.argv[argNumber] == "nothing":
			nothing = True

if not nothing:
	if not onlySimple:
		times = []

		for i in range(0, NUM_TESTS):
			start_time = getTime()

		#	subprocess.call([STITCHED_PROGRAM, STITCHED_PROGRAM_ARGS])
			os.system(STITCHED_PROGRAM + STITCHED_PROGRAM_ARGS);

			end_time = getTime()

			timediff = end_time - start_time

			print ("Run (stitched) " + str(i) + " completed - " + str(timediff));

			if i >= NUM_DISCARD:
				times.append(timediff)	

			time.sleep(1.0)

		writeTimes(times, "results_stitched.txt")

	if doSimple:
		time.sleep(5)
		times = []

		for i in range(0, NUM_TESTS):
			start_time = getTime()

			#subprocess.call([SIMPLE_PROGRAM, SIMPLE_PROGRAM_ARGS])
			#try:
			#print ( subprocess.check_output(SIMPLE_PROGRAM))
			#except subprocess.CalledProcessError as e:
			#	print ("Stdout output:\n", e.cmd)
			os.system(SIMPLE_PROGRAM + SIMPLE_PROGRAM_ARGS);

			end_time = getTime()

			timediff = end_time - start_time

			print ("Run (simple) " + str(i) + " completed - " + str(timediff));

			if i >= NUM_DISCARD:
				times.append(timediff)

			time.sleep(1.0)

		writeTimes(times, "results_simple.txt")

currTime = time.strftime("%y_%m_%d_%H_%M_%S")

copy2("results_simple.txt", "logs/results_simple_" + currTime + ".txt")
copy2("results_stitched.txt", "logs/results_stitched_" + currTime + ".txt")

file = open("logs/results_info_" + currTime + ".txt","w") 
file.write("SIMPLE PROGRAM = " + SIMPLE_PROGRAM);
file.write("\n");		
file.write("STITCHED PROGRAM = " + STITCHED_PROGRAM);
file.write("\n");	
file.write("SIMPLE PROGRAM ARGS = " + SIMPLE_PROGRAM_ARGS);
file.write("\n");	
file.write("STITCHED PROGRAM ARGS = " + STITCHED_PROGRAM_ARGS);
file.write("\n");	
file.close()


os.system("ministat results_simple.txt results_stitched.txt")
