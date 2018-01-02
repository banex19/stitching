from scipy import stats
from scipy.stats import iqr
from numpy import median
import sys

if len(sys.argv) != 3:
	print("usage: wilcoxon.py <dataset1> <dataset2>")
	exit(1)

file1 = sys.argv[1]
file2 = sys.argv[2]

dataset1 = []
dataset2 = []

with open(file1) as f:
	dataset1 = f.readlines()

with open(file2) as f:
	dataset2 = f.readlines()

dataset1 = map(float, dataset1)
dataset2 = map(float, dataset2)
median1 = round(median(dataset1),4)
median2 = round(median(dataset2),4)
iqr1 = round(iqr(dataset1),4)
iqr2 = round(iqr(dataset2),4)


pvalue = stats.ranksums(dataset1,dataset2)[1]

print("Dataset 1: Median " + str(median1) + " - IQR " + str(iqr1))
print("Dataset 2: Median " + str(median2) + " - IQR " + str(iqr2))
print("% gain: " + str(round((1 - median1/median2) * -100.0, 2)) + "%")


print("P value: " + str(pvalue))
print("95% confidence: " + str(pvalue < 0.05))
print("99% confidence: " + str(pvalue < 0.01))
print("99.5% confidence: " + str(pvalue < 0.005))