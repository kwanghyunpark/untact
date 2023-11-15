// ARIMAPredictor.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../CommonCodes/arima_tool.h"

double ar_param[] =
{
	1.258051254149946,
	0.227577311182717,
	-0.258738773793149,
	-0.147970081076526,
	0.001664111920015,
	-0.115861421910991,
	-0.001683762286085,
	0.001195693456229,
	-0.100830448025619,
	0.132442431599762
};

double y[]={
	88.995293000000004,
	92.642166000000003,
	94.392807000000005,
	94.869179000000003,
	94.783867000000001,
	93.506400999999997,
	90.911582999999993,
	87.830878999999996,
	84.190651000000003,
	82.197425999999993,
	78.231316000000007,
	74.348450000000000,
	72.510727000000003,
	69.057243000000000,
	67.531738000000004,
	63.265563999999998,
	61.500435000000003,
	57.994053000000001,
	54.590656000000003,
	51.264190999999997
};

int _tmain(int argc, _TCHAR* argv[])
{
	ARIMAModel arima;
	for(int i=0; i<10; i++){
		arima.ar_params.push_back(ar_param[i]);
	}
	arima.intercept = 0.067000893254963;
	arima.sigma_2 = 0.129540203581745;
	arima.d = 0;

	unsigned int N = 10;
	std::vector<double> observations;

	for(int i=0; i<20; i++){
		observations.push_back(y[i]);
	}

	Predictions output = ARIMAForecaster::forecast(arima, N, observations);
	for(int i=0; i< output.values.size(); i++){
		printf("%f \n", output.values[i]);
	}

	
	return 0;
}

