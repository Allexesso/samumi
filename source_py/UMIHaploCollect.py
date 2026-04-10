import sys
import math

for inarg in sys.argv[1:]:
    if (inarg == "--help") or (inarg == "-h") or (inarg == "/?"):
        print("Collects UMI family data into haplotypes.")
        print("Expects input data over stdin.")
        print("Spits output onto stdout.")
        sys.exit(0)
    if inarg == "--version":
        print("SAMmed UMI 0.0")
        print("Copyright (C) 2021 UNT Center for Human Identification")
        print("License LGPLv3+: GNU LGPL version 3 or later")
        print("    <https://www.gnu.org/licenses/lgpl-3.0.html>")
        print("This is free software: you are free to change and redistribute it.")
        print("There is NO WARRANTY, to the extent permitted by law.")
        sys.exit(0)
    raise ValueError("Unknown argument: " + inarg)


def safeLog(forV):
    if forV == 0.0:
        return -math.inf
    return math.log(forV)

def safeLog10(forV):
    if forV == 0.0:
        return -math.inf
    return math.log10(forV)

def log10ProbComp(forV):
    workV = forV / math.log10(math.e)
    if workV < 0.693:
        workV = safeLog(- math.expm1(workV))
    else:
        workV = math.log1p(- math.exp(workV))
    return workV / math.log(10.0)

allHaplos = {}
locTotUmi = {}
locTotRead = {}

# load it in
firstL = True
colMap = {}
primInd = 1
alleInd = 7
redcInd = 8
for line in sys.stdin:
    if line.strip() == "":
        continue
    lineS = [cv.strip() for cv in line.split("\t")]
    if firstL:
        firstL = False
        for i in range(len(lineS)):
            colMap[lineS[i]] = i
        primInd = colMap["Primer"]
        alleInd = colMap["Primary_Allele"]
        redcInd = colMap["Primary_Read_Count"]
        aredcInd = colMap["Reads"]
        continue
    curPrim = lineS[primInd]
    curKey = (lineS[primInd], lineS[alleInd])
    if not (curKey in allHaplos):
        allHaplos[curKey] = []
    if not (curPrim in locTotUmi):
        locTotUmi[curPrim] = 0
        locTotRead[curPrim] = 0
    allHaplos[curKey].append((int(lineS[redcInd]), lineS))
    locTotUmi[curPrim] += 1
    locTotRead[curPrim] += int(lineS[aredcInd])

print(locTotUmi)
print(locTotRead)

# output families
toPrin = []
toPrin.extend(["Locus","Period","Primer","Primary_Allele","Primary_Length"])
toPrin.extend(["UMI_Count","Total_Primary_Read_Count","Total_Reads"])
toPrin.extend(["Log10Error","Read_Diff","PhredAggError"])
toPrin.extend(["F0_Primary_Reads","F0_Purity","F0_MaxMinQ_UMI","F0_Best_PNoSeqErr"])
toPrin.extend(["F1_Primary_Reads","F1_Purity","F1_MaxMinQ_UMI","F1_Best_PNoSeqErr"])
toPrin.extend(["F2_Primary_Reads","F2_Purity","F2_MaxMinQ_UMI","F2_Best_PNoSeqErr"])
toPrin.extend(["F3_Primary_Reads","F3_Purity","F3_MaxMinQ_UMI","F3_Best_PNoSeqErr"])
toPrin.extend(["F4_Primary_Reads","F4_Purity","F4_MaxMinQ_UMI","F4_Best_PNoSeqErr"])
toPrin.extend(["Bulk_Purity"])
toPrin.extend(["UMI_per_Locus_Total_UMI", "Reads_per_Locus_Total_Reads"])
print("\t".join(toPrin))
if len(allHaplos) == 0:
    sys.exit(0)
locInd = colMap["Locus"]
perInd = colMap["Period"]
lenInd = colMap["Primary_Length"]
phrAggInd = colMap["Phred_Aggregate"]
pcorInd = phrAggInd  # pCorrect column was merged into Phred_Aggregate
propInd = colMap["Primary_Proportion"]
maxminUInd = colMap["Phred_MaxMin_UMI"]
phrAggBestInd = colMap["Phred_Aggregate_Best_Primary"]
aredcInd = colMap["Reads"]
famDat = []
for curKey in allHaplos:
    toPrin.clear()
    famDat.clear()
    curPrim = curKey[0]
    curAlles = allHaplos[curKey]
    curAlles.sort()
    curAlles.reverse()
    hapLen = int(curAlles[0][1][lenInd])
    hapPer = curAlles[0][1][perInd]
    hapLoc = curAlles[0][1][locInd]
    numUmi = len(curAlles)
    numRead = 0
    totRead = 0
    phredAggWrong = 0.0
    pnotCor = 0.0
    for alleT in curAlles:
        lineS = alleT[1]
        numRead += alleT[0]
        totRead += int(lineS[aredcInd])
        phredAggWrong += log10ProbComp(float(lineS[phrAggInd]))
        pnotCor += log10ProbComp(float(lineS[pcorInd]))
        if len(famDat) == 20:
            continue
        famDat.append(lineS[redcInd])
        famDat.append(lineS[propInd])
        famDat.append(lineS[maxminUInd])
        famDat.append(lineS[phrAggBestInd])
    while len(famDat) != 20:
        famDat.append("0")
        famDat.append("0.0")
        famDat.append("1")
        famDat.append(repr(hapLen * log10ProbComp(-0.1)))
    bulkPure = numRead / totRead
    toPrin.append(hapLoc)
    toPrin.append(hapPer)
    toPrin.append(curKey[0])
    toPrin.append(curKey[1])
    toPrin.append(repr(hapLen))
    toPrin.append(repr(numUmi))
    toPrin.append(repr(numRead))
    toPrin.append(repr(totRead))
    toPrin.append(repr(pnotCor))
    toPrin.append(repr(totRead - numRead))
    toPrin.append(repr(phredAggWrong))
    toPrin.extend(famDat)
    toPrin.append(repr(bulkPure))
    toPrin.append(repr(numUmi / locTotUmi[curPrim]))
    toPrin.append(repr(totRead / locTotRead[curPrim]))
    print("\t".join(toPrin))

