#include "VectorAR.h"

#define MAX_BUF 4096


std::vector<double> parseOneLine(const char* lineStr, char* sep)
{
	int nDataCnt = 0;
	std::vector<double> data;

	char* tmpStr = new char[sizeof(char)*(strlen(lineStr)+1)];
	strcpy(tmpStr, lineStr);

	char* token = strtok(tmpStr, sep);
	while(token != NULL){
		if(token[0] != 10){
			data.push_back(atof(token));	
		}
		token = strtok(NULL, sep);
	}	
	delete [] tmpStr;

	return data;
}


/////////////////////////////////////////////////////////////////////////////////////
// ==================================================================================
//    VARTraner class (base class)
// ----------------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////////

bool VARTrainer::constructModelFromCoeffsFile(char* file)
{
	char line[MAX_BUF];
	FILE* fp = fopen(file, "r");
	if(fp == NULL){ return false;}

	std::vector<std::vector<double>> data;

	int noCols = 0;
	bool isFirst = true;

	while(!feof(fp)){
		while ( fgets ( line, MAX_BUF, fp ) != NULL ) // read a line 
		{
			vector<double> elements = parseOneLine(line, " ,");
			if(isFirst){
				noCols = elements.size();
				isFirst = false;
			}
			if(noCols != elements.size()){
				printf("Data parsing failure. The number of columns is mismatched..");
				fclose(fp);
				return false;
			}
			data.push_back(elements);
		}      
	}
	fclose(fp);

	// construct coeffs matrix
	int noRows = data.size();
	Mat Coefs(noRows, noCols, CV_64FC1);

	for(int i=0; i<noRows; i++){
		for(int j=0; j<noCols; j++){
			Coefs.at<double>(i,j) = data[i][j];
		}
	}
	// create model by coeffs matrix
	this->model.setcoefMat(Coefs);

	return true;
}

bool VARTrainer::saveCoeffsAsFile(char* file)
{
	Mat coefs = this->model.getCoefMat();
	FILE* fp = fopen(file, "w");
	if(fp == NULL){ return false;}
	
	for(int i=0; i<coefs.rows; i++){
		for(int j=0; j<coefs.cols; j++){
			if(j == coefs.cols-1){
				fprintf(fp, "%.20f", coefs.at<double>(i,j));
			}else{
				fprintf(fp, "%.20f, ", coefs.at<double>(i,j));
			}
		}
		fprintf(fp, "\n");
	}

	fclose(fp);

	return false;
}

vector<vector<double>> VARTrainer::predict(list<vector<double>>& inputStream, int stepAhead)
{
	vector<vector<double>> pred;

	int p = model.getAROrder();
	if(inputStream.size() < p){
		return vector<vector<double>>();
	}
	
	//static int64_t t1 = av_gettime();

	vector<vector<double>> dataStream(p);
	int cnt = p;
	for(list<vector<double>>::reverse_iterator itor = inputStream.rbegin(); itor != inputStream.rend(), cnt > 0; ++itor){
		dataStream[--cnt] = *itor;
	}


	int t = dataStream.size() - 1;
	if(t-p+1 < 0 || dataStream.empty()){
		printf("cannot compute the predicted values.\n");
		return vector<vector<double>>();
	}
	int n_var = model.getNoVariables();
	cv::Mat ref_data(n_var, p+stepAhead, CV_64FC1);
	for(int i=0; i<n_var; i++){
		for(int j=0; j<p; j++){
			ref_data.at<double>(i,j) = dataStream[t-p+1 + j][i];
		}
	}

	// prediction 
	for(int k=0; k<stepAhead; k++){
		model.getAR()[0].copyTo(ref_data.col(k+p));

		for (int j=0; j<p; j++){
			int aa = k+p-j-1;
			ref_data.col(k+p) += model.getAR()[j+1]*ref_data.col(k+p-j-1);
		}

		// set predicted value 
		vector<double> aVector;
		for(int i=0; i<n_var; i++){
			aVector.push_back(ref_data.at<double>(i, k+p));
		}
		pred.push_back(aVector);
	}

	//static int oldt = 0;
	//int64_t t2 = av_gettime();
	//printf("time = %Ld \n", (t2-oldt));
	//oldt = t2;

	return pred;
}


/////////////////////////////////////////////////////////////////////////////////////
// ==================================================================================
//    VARBatchTraner class
// ----------------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////////

bool VARBatchTrainer::loadFromDataFile(char* file, int order_p, vector<int> idxFeatures)
{
	FILE* fp = fopen(file, "r");
	char line[MAX_BUF];

	if(fp == NULL){
		printf("check the first argument which should be input file name.\n");
		return false;
	}
	
	std::vector<std::vector<double>> data;

	int noElements = 0;
	bool isFirst = true;

	while(!feof(fp)){
		while ( fgets ( line, MAX_BUF, fp ) != NULL ) // read a line 
		{
			vector<double> elements = parseOneLine(line, ", ");
			if(isFirst){
				noElements = elements.size();
				isFirst = false;
			}
			if(noElements != elements.size()){
				printf("Data parsing failure. The number of columns is mismatched..");
				fclose(fp);
				return false;
			}
			data.push_back(elements);
		}      
	}
	fclose(fp);

	std::vector<int> idxs;
	if(data.empty()){ return false;}
	if(idxFeatures.empty()){
		for(int i=0; i< data.at(0).size(); i++){idxs.push_back(i);}
	}else{ idxs = idxFeatures;}

	for(int i=0; i< idxs.size(); i++){ 
		if(idxs[i] >= data.at(0).size()){		
			return false;
		}
	}

	this->dataSet.clear();
	for(int i=0; i< data.size(); i++){
		vector<double> aRowVector;
		for(int j=0; j<idxs.size(); j++){
			aRowVector.push_back(data[i][idxs[j]]);
		}
		this->dataSet.push_back(aRowVector);
	}

	// set VAR model
	this->model = VARModel(order_p, idxs.size());
	
	return true;
}

bool VARBatchTrainer::train(void*)
{
	if(dataSet.empty()){
		return false;
	}

	int nDimensions = model.getNoVariables();
	int order_p = model.getAROrder();
	int nDataPoints = this->dataSet.size();

	vector<vector<double>> trainingData;
	for(list<vector<double>>::iterator iter = dataSet.begin(); iter != dataSet.end(); ++iter){
		trainingData.push_back(*iter);
	}

	cv::Mat Z(nDimensions, nDataPoints-order_p, CV_64FC1);
	cv::Mat A(1+order_p*nDimensions, nDataPoints-order_p, CV_64FC1);

	for(int i=0; i<nDataPoints-order_p; i++){
		// For the equation Z = A*b, set Z first,
		for(int k=0; k<nDimensions; k++){   
			Z.at<double>(k, i) = trainingData[i+order_p][k];	
		}
		// then, set A for every data vector
		A.at<double>(0, i) = 1.0;
		for(int j=0; j<order_p; j++){
			for (int k=0; k<nDimensions; k++){
				A.at<double>(1 + (j*nDimensions) + k, i) = trainingData[i+order_p -j-1][k];
			}
		}
		/*
		printf("------ %d \n", i+1);
		for (int ii=0; ii< Z.rows; ii++){
			printf("%.20f, ", Z.at<double>(ii,i));
		}
		printf("\n");

		for (int ii=0; ii< A.rows; ii++){
			printf("%.20f, ", A.at<double>(ii,i));
		}
		printf("\n");
		getchar();
		*/
	}
	/*
	Mat AAt = (A * A.t());
	Mat ZAt = Z * A.t();

	FILE *fp;
	fp = fopen("Z.txt","w");
	for (int ii=0; ii< Z.rows; ii++){
		for (int jj=0; jj< Z.cols; jj++){
			fprintf(fp, "%.20f, ", Z.at<double>(ii,jj));
		}
		fprintf(fp, "\n");
	}
	fclose(fp);

	FILE *fp1;
	fp1 = fopen("A.txt","w");
	for (int ii=0; ii< A.rows; ii++){
		for (int jj=0; jj< A.cols; jj++){
			fprintf(fp1, "%.20f, ", A.at<double>(ii,jj));
		}
		fprintf(fp1, "\n");
	}
	fclose(fp1);
	
	FILE *fp2;
	fp2 = fopen("AAt.txt","w");
	for (int ii=0; ii< AAt.rows; ii++){
		for (int jj=0; jj< AAt.cols; jj++){
			fprintf(fp2, "%.20f, ", AAt.at<double>(ii,jj));
		}
		fprintf(fp2, "\n");
	}
	fclose(fp2);

	FILE *fp3;
	fp3 = fopen("ZAt.txt","w");
	for (int ii=0; ii< ZAt.rows; ii++){
		for (int jj=0; jj< ZAt.cols; jj++){
			fprintf(fp3, "%.20f, ", ZAt.at<double>(ii,jj));
		}
		fprintf(fp3, "\n");
	}
	fclose(fp3);
	*/
	Mat coefMat = Z * A.t() * (A * A.t()).inv();

	model.setcoefMat(coefMat);

	return true;
};


/////////////////////////////////////////////////////////////////////////////////////
// ==================================================================================
//    VAROnlineTrainer class
// ----------------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////////
void VAROnlineTrainer::setInitialParams(double learning_rate, double momentum, Mat coefMat)
{
	this->learning_rate = learning_rate;
	this->momentum = momentum;
	if(!coefMat.empty()){
		this->model.setcoefMat(coefMat);
	}else{
		// fill the coefficient matrices with random numbers 
	}

	dw_old = Mat(this->model.getCoefMat().rows, this->model.getCoefMat().cols, CV_64FC1, cv::Scalar::all(0));
	online_step = 0;
}

bool VAROnlineTrainer::train(void* argin)
{
	bool flag = false;

	vector<double>* aFeatureVector = (vector<double>*) argin;
	dataSet.push_back(*aFeatureVector);

	int nDimensions = model.getNoVariables();
	int order_p = model.getAROrder();

	if(online_step >= order_p){

		cv::Mat Z(nDimensions, 1, CV_64FC1);
		cv::Mat A(1+order_p*nDimensions, 1, CV_64FC1);


		int cnt = order_p + 1; // previous data (p) + latest data (1) 
		vector<vector<double>> dataStream(cnt);

		for(list<vector<double>>::reverse_iterator itor = dataSet.rbegin(); itor != dataSet.rend(), cnt > 0; ++itor){
			dataStream[--cnt] = *itor;
		}

		int endIdx = order_p;
		for(int k=0; k<nDimensions; k++){
			Z.at<double>(k, 0) = dataStream[endIdx][k];	
		}
		//cout << Z.row(i) << endl;

		// set A
		A.at<double>(0, 0) = 1.0;
		for(int j=0; j<order_p; j++){
			for (int k=0; k<nDimensions; k++){
				int kk = endIdx -j -1;
				A.at<double>(1 + (j*nDimensions) + k, 0) = dataStream[endIdx -j-1][k];
			}
		}
		Mat w = model.getCoefMat();
		//double lr = 0.01*learning_rate;

		Mat grad = (w*A - Z)*A.t();
		Mat dw = momentum*dw_old - learning_rate*grad/cv::norm(grad);
		w = w + dw;
		dw_old = dw;

		// 			Mat disp = w.t();
		// 			for (int ii=0; ii<disp.rows; ii++){
		// 				for (int jj=0; jj<disp.cols; jj++){
		// 					printf("%.4f, ", disp.at<double>(ii,jj));
		// 				}
		// 				printf("\n");
		// 			}
		//printf("\n");
		//cout << setprecision(4) << betaOnline.t() <<  endl;
		//getchar();
		flag = true;
	}

	online_step++;

	return flag;
}


