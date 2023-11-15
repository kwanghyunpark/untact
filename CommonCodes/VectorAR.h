#pragma once

#include "opencv2/opencv.hpp"
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <windows.h>
# include "stdint.h"


using namespace std;
using namespace cv;

class VARModel
{
private:
	int p;
	int nVar;
	Mat coefMat;
	vector<Mat> AR;
	Mat sigma;

protected:

public:
	VARModel(){ buildModel(1,1); };
	VARModel(int p, int nVar) {	buildModel(p,nVar);};
	~VARModel() { release();};
	
	void buildModel(int p, int nVar)
	{
		if(p != this->p || nVar != this->nVar){
			release();
		}

		this->p = p;
		this->nVar = nVar;
		coefMat = Mat(nVar, p*nVar+1, CV_64FC1, cv::Scalar::all(0));
		AR.resize(p+1);
		AR[0] = Mat(nVar, 1, CV_64FC1, Scalar::all(0));
		for(int i=0; i<p; i++){
			AR[i+1] = Mat(nVar, nVar, CV_64FC1, Scalar::all(0));
		}
		sigma = Mat(p*nVar+1, p*nVar+1, CV_64FC1);
	};
	
	void buildModel(Mat coefMat){
		setcoefMat(coefMat);
	}

	void release()
	{
		if(!coefMat.empty()) {coefMat.release();}

		if(!AR.empty()){
			AR[0].release();
			for(int i=0; i<p; i++){
				AR[i+1].release();
			}
			AR.clear();
		}

		if(!sigma.empty()) {sigma.release();}
	};

	void setcoefMat(Mat coefMat)
	{
		if(coefMat.empty()){return;};
		int aa = p*nVar+1;
		if(nVar != coefMat.rows || p*nVar+1 != coefMat.cols){
			int bb = ((coefMat.cols-1)/coefMat.rows);
			buildModel((int) ((coefMat.cols-1)/coefMat.rows), coefMat.rows);
		}
		coefMat.copyTo(this->coefMat);
		this->AR[0] = coefMat.col(0); 
		for(int i=0; i<this->p; i++){
			this->AR[i+1] = coefMat.colRange(cv::Range(1 + i*this->nVar, 1 + (i+1)*this->nVar));
		}
	};
	void setSigma(Mat sigma){sigma.copyTo(this->sigma);};	
	int getAROrder(){return this->p;};
	int getNoVariables(){return this->nVar;};
	Mat& getCoefMat(){return this->coefMat;};
	vector<Mat>& getAR(){return this->AR;};
	Mat& getSigma(){return sigma;};

	VARModel& operator= (VARModel& rhs)
	{
		if(this != &rhs){
			//setModelStructure(rhs.getAROrder(), rhs.getNoVariables());
			setcoefMat(rhs.getCoefMat());
			setSigma(rhs.getSigma());	
		}
		return *this;
	};

	void showModelStructure()
	{
		printf("Vector AR Model [p=%d, n_var=%d]\n", this->p, this->nVar);
	}
};

class VARTrainer
{
protected:
	VARModel model;
	std::list<vector<double>> dataSet;

public:
	VARTrainer(){};
	virtual ~VARTrainer(){};

	vector<vector<double>> predict(list<vector<double>>& dataStream, int stepAhead);
	void setModel(VARModel model){this->model = model;};
	VARModel& getModel(){return model;};
	std::list<vector<double>>& getDataSet(){return dataSet;};
	bool constructModelFromCoeffsFile(char* file);
	bool saveCoeffsAsFile(char* file);

	virtual bool train(void*) = NULL;
};

class VARBatchTrainer : public VARTrainer
{
private:

protected:

public:
	VARBatchTrainer(){};
	VARBatchTrainer(VARModel* model){ this->model = *model;};
	~VARBatchTrainer(){};
		
	bool loadFromDataFile(char* file, int order_p, vector<int> idxFeatures = vector<int>());
	bool train(void* = NULL);
	void addFeatureVector(vector<double> aDataVector){ dataSet.push_back(aDataVector);};
};

class VAROnlineTrainer : public VARTrainer
{
private:
	double learning_rate;
	double momentum;
	int online_step;
	Mat dw_old;

protected:

public:
	VAROnlineTrainer(){};
	VAROnlineTrainer(VARModel* model){ this->model = *model;};
	~VAROnlineTrainer(){};
	void setInitialParams(double learning_rate, double momentum, Mat coefMat = Mat());
	bool train(void* argin);
};

