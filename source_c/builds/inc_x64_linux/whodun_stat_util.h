#ifndef WHODUN_STAT_UTIL_H
#define WHODUN_STAT_UTIL_H 1

namespace whodun {

#define WHODUN_PROB_SUMMATION_QUEUE 50

/**A utility for adding up probabilities.*/
class ProbabilitySummation{
public:
	/**Set up an empty sum.*/
	ProbabilitySummation();
	/**Clear out the queued values.*/
	void updateFromQueue();
	/**
	 * Add a value to the queue.
	 * @param nextLogProb The log10 of the probability to add.
	 */
	void addNextLogProb(double nextLogProb);
	/**
	 * After all values have been managed, get the final value.
	 * @return The log10 of the sum of the probabilities. If P is the maximum probability, calculated as log10(P) + log(sum(10^(log10(p)-log10(P))))
	 */
	double getFinalLogSum();
	/**The next queue end.*/
	int nextQInd;
	/**The running maximum probability.*/
	double maxLogProb;
	/**The running sum of shifted probabiliites.*/
	double runningSum;
	/**Queued values.*/
	double queuedVals[WHODUN_PROB_SUMMATION_QUEUE];
};

/**
 * Calculate the log gamma function.
 * @param forVal The value to calculate for.
 * @return The ln-gamma.
 */
double logGamma(double forVal);

/**
 * Calculate the log gamma function to some level of precision.
 * @param forVal The value to calculate for.
 * @param useEpsilon The precision.
 * @return The calculated value.
 */
double logGammaT(double forVal, double useEpsilon);

/**
 * Calculate exp(x)-1.
 * @param forVal The value to calculate for.
 * @return The value, with more precision than naive.
 */
double expMinusOne(double forVal);

/**
 * Calculate exp(x)-1 to some level of precision..
 * @param forVal The value to calculate for.
 * @param useEpsilon The precision.
 * @return The value, with more precision than naive.
 */
double expMinusOneT(double forVal, double useEpsilon);

/**
 * Calculate ln(1+x).
 * @param forVal The value to calculate for.
 * @return The value, with more precision than naive.
 */
double logOnePlus(double forVal);

/**
 * Calculate ln(1+x) to some level of precision.
 * @param forVal The value to calculate for.
 * @param useEpsilon The precision.
 * @return The value, with more precision than naive.
 */
double logOnePlusT(double forVal, double useEpsilon);

/**
 * Calculate log(1 - exp(-a)).
 * @param forVal The value to compute for.
 * @return The value, with more precision than naive.
 */
double logOneMinusExp(double forVal);

/**
 * Calculate log(1 - exp(-a)) to some level of precision.
 * @param forVal The value to compute for.
 * @param useEpsilon The precision.
 * @return The value, with more precision than naive.
 */
double logOneMinusExpT(double forVal, double useEpsilon);

}

#endif
