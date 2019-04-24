#include "ConfidenceMap.h"

namespace topology_map {

/*************************************************
Function: Confidence
Description: constrcution function for Confidence class
Calls: SetSigmaValue
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: f_fSigma - the parameter sigma of GaussianKernel
Output: none
Return: none
Others: none
*************************************************/
Confidence::Confidence(float f_fSigma,
	                   float f_fGHPRParam,
	                  float f_fVisTermThr):
	                     m_fWeightDis(0.7),
                         m_fWeightVis(0.3),
	                      m_fDensityR(0.3),
                         m_fLenWeight(0.6),
                       m_fBoundWeight(0.4){

	SetSigmaValue(f_fSigma);

	SetVisParas(f_fGHPRParam, f_fVisTermThr);

	//srand((unsigned)time(NULL));

}


/*************************************************
Function: ~Confidence
Description: destrcution function for Confidence class
Calls: all member functions
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: none
Output: none
Return: none
Others: none
*************************************************/
Confidence::~Confidence(){


}

/*************************************************
Function: SetSigmaValue
Description: set value to the private data member m_fSigma
Calls: none
Called By: Confidence
Table Accessed: none
Table Updated: none
Input: f_fSigma - a given sigma value depends on the scanning region
Output: none
Return: none
Others: none
*************************************************/
void Confidence::SetSigmaValue(const float & f_fSigma) {

	m_fSigma = f_fSigma;

}

/*************************************************
Function: SetVisParas
Description: set value to the parameters related to visibility term
Calls: none
Called By: Confidence
Table Accessed: none
Table Updated: none
Input: f_fGHPRParam - a parameter of GHPR algorithm
      f_fVisTermThr - a threshold of visibility value
Output: m_fGHPRParam
        m_fVisTermThr
Return: none
Others: none
*************************************************/
void Confidence::SetVisParas(const float & f_fGHPRParam, 
	                         const float & f_fVisTermThr){

	m_fGHPRParam = f_fGHPRParam;

	m_fVisTermThr = f_fVisTermThr;

}


/*************************************************
Function: VectorInnerProduct
Description: This is an inner product operation of two vectors
Calls: none
Called By: DistanceTerm
Table Accessed: none
Table Updated: none
Input: oAVec - vector A
       oBVec - vector B
Output: the inner product value of vector A and vector B
Return: an inner product value
Others: none
*************************************************/
float Confidence::VectorInnerProduct(const pcl::PointXYZ & oAVec,
	                                 const pcl::PointXYZ & oBVec){

	//a*b = xa*xb + ya*yb + za*zb
	return oAVec.x * oBVec.x + oAVec.y * oBVec.y + oAVec.z * oBVec.z;

}

/*************************************************
Function: GaussianKernel
Description: This is a Gaussian Kernel Function to smooth the distance value between two points
Calls: Compute2Norm
Called By: DistanceTerm
Table Accessed: none
Table Updated: none
Input: oQueryPo - the query point (based point)
       oTargerPo - the target point 
	   sigma - the parameter that controls the affect radius of Gaussian Function, therefore the value
	   of sigma is normally set as half of searched radius 
Output: none
Return: none
Others: none
*************************************************/
inline float Confidence::GaussianKernel(const pcl::PointXYZ & oQueryPo,
	                                    const pcl::PointXYZ & oTargerPo, 
	                                                      float & sigma){

	// k(|| x - xc || ) = exp{ -|| x - xc || ^ 2 / (2 * sigma^2) }
	//or k(|| x - xc || ) = exp{ -|| x - xc || ^ 2 / (sigma^2) }

	//distance between two input vector
	float fNormSquare = Compute2Norm(oQueryPo, oTargerPo);
	
	      fNormSquare = pow(fNormSquare, 2.0f);
	//ouput equation result
	return exp(-1.0f * fNormSquare / pow(sigma,2.0f));
	
}

/*************************************************
Function: LinearKernel
Description: This is a Linear Kernel Function to compute the visibility value 
Calls: none
Called By: 
Table Accessed: none
Table Updated: none
Input: fTargetVal - input of linear function
	   fThrVal - the threshold of linear function,
	   the value will be 1 if input is larger than this one 
Output: the respond of linear function
Return: float computed value
Others: none
*************************************************/
inline float Confidence::LinearKernel(const float & fTargetVal,
	                         const float & fThrVal){

	//if the input value is smaller than the given threshold 
	if (fTargetVal < fThrVal)
		return fTargetVal / fThrVal;
	else
		return 1.0;

}

/*************************************************
Function: StandardDeviation
Description: This is a Linear Kernel Function to compute the visibility value
Calls: none
Called By:
Table Accessed: none
Table Updated: none
Input: fTargetVal - input of linear function
          fThrVal - the threshold of linear function,
          the value will be 1 if input is larger than this one
Output: the respond of linear function
Return: float computed value
Others: none
*************************************************/
float Confidence::StandardDeviation(const PCLCloudXYZ & vCloud){

	//define output
	float fSDeviation = 0.0;

	//compute the mean value of point set
	pcl::PointXYZ oMeanPoint = ComputeCenter(vCloud);
	for (int i = 0; i != vCloud.points.size(); ++i) {
	    //accumulation
		fSDeviation += ComputeSquareNorm(oMeanPoint, vCloud.points[i]);

	}

	//compute the standard deviation
	fSDeviation = sqrt(fSDeviation/ float(vCloud.points.size()));

	return fSDeviation;

}

/*************************************************
Function: ComputeDensity
Description: This is a Linear Kernel Function to compute the visibility value
Calls: none
Called By:
Table Accessed: none
Table Updated: none
Input: fTargetVal - input of linear function
fThrVal - the threshold of linear function,
the value will be 1 if input is larger than this one
Output: the respond of linear function
Return: float computed value
Others: none
*************************************************/
inline float Confidence::ComputeDensity(const PCLCloudXYZ & vCloud,
								                   int iSampleTimes,
								                       bool bKDFlag){

	//use the kdtree structureto compute density if bKDFlag is true
	if(bKDFlag){

		//if point number is very small
		if(vCloud.points.size() < iSampleTimes)
			return 1.0;//it is neighborhood is onlt itself
		
		PCLCloudXYZPtr pGridCloud(new PCLCloudXYZ);
		//construct a point clouds
	    pGridCloud->width = vCloud.points.size();
	    pGridCloud->height = 1;
	    pGridCloud->is_dense = false;
	    pGridCloud->points.resize(pGridCloud->width * pGridCloud->height);

		for(int i = 0;i != vCloud.size(); ++i){

			pGridCloud->points[i].x = vCloud.points[i].x;
			pGridCloud->points[i].y = vCloud.points[i].y;
			pGridCloud->points[i].z = vCloud.points[i].z;
		}

		//construct a kdtree
	    pcl::KdTreeFLANN<pcl::PointXYZ> oGridCloudTree;
	    oGridCloudTree.setInputCloud(pGridCloud);
		
		//get the random query index of point
		std::vector<int> vQueryIndices = GetRandom(pGridCloud->points.size(), iSampleTimes);

		//total point number
		int iNeighPNums = 0;

		//find indices using kdtree
	    for (size_t i = 0; i != vQueryIndices.size(); ++i) {
			
			//define temps
			std::vector<int> vNearestIdx;
			std::vector<float> vNearestDis;
			
			//search the nearest raw point of query constructed convex hull surface point 
			oGridCloudTree.radiusSearch(pGridCloud->points[vQueryIndices[i]], 0.3, vNearestIdx, vNearestDis);
			
			//accumulation
			iNeighPNums += vNearestIdx.size();
		
		}//end for i != pHullCloud->points.size()

		return float(iNeighPNums)/float(iSampleTimes);

	}else{//ouput the point number directly in false model
	
		return float(vCloud.points.size());
	}

}



/*************************************************
Function: Compute2Norm
Description: compute the Euclidean distance between two points
Calls: none
Called By: GaussianKernel
Table Accessed: none
Table Updated: none
Input: oQueryPo - the query point (based point)
       oTargerPo - the target point 
Output: none
Return: none
Others: This function is the same with ComputeEuclideanDis, but it is an online one
*************************************************/
inline float Confidence::Compute2Norm(const pcl::PointXYZ & oQueryPo, const pcl::PointXYZ & oTargerPo){

	//compute the eucliden distance between the query point (robot) and target point (scanned point)
	return sqrt(pow(oQueryPo.x - oTargerPo.x, 2.0f)
		      + pow(oQueryPo.y - oTargerPo.y, 2.0f)
		      + pow(oQueryPo.z - oTargerPo.z, 2.0f));

}

/*************************************************
Function: ComputeSquareNorm
Description: compute the square of norm
Calls: none
Called By: GaussianKernel
Table Accessed: none
Table Updated: none
Input: oQueryPo - the query point (based point)
       oTargerPo - the target point 
Output: the Euclidean distance value between two points
Return: a distance value 
Others: This function is the same with ComputeEuclideanDis, but it is an online one
*************************************************/
inline float Confidence::ComputeSquareNorm(const pcl::PointXYZ & oQueryPo, const pcl::PointXYZ & oTargerPo){

	return pow(oQueryPo.x - oTargerPo.x, 2.0f)
		     + pow(oQueryPo.y - oTargerPo.y, 2.0f)
	         + pow(oQueryPo.z - oTargerPo.z, 2.0f);

}

/*************************************************
Function: ComputeCenter
Description: compute the center point of a given point set with quired index
Calls: none
Called By: DistanceTerm
Table Accessed: none
Table Updated: none
Input: vCloud - a 3D point clouds
       vPointIdx - the point index vector indicates which point need to be computed 
Output: centerpoint center point(position) of the queried point set
Return: centerpoint
Others: none
*************************************************/
pcl::PointXYZ Confidence::ComputeCenter(const PCLCloudXYZ & vCloud,
	                            const std::vector<int> & vPointIdx){

	//define output
	pcl::PointXYZ oCenter;
	oCenter.x = 0.0;
	oCenter.y = 0.0;
	oCenter.z = 0.0;

	//check whether the input has point 
	if(!vPointIdx.size())
		return oCenter;

	//calculate the average
	for (int i = 0; i != vPointIdx.size(); ++i) {

		oCenter.x = oCenter.x + vCloud.points[vPointIdx[i]].x;
		oCenter.y = oCenter.y + vCloud.points[vPointIdx[i]].y;
		oCenter.z = oCenter.z + vCloud.points[vPointIdx[i]].z;

	}

	oCenter.x = oCenter.x / float(vPointIdx.size());
	oCenter.y = oCenter.y / float(vPointIdx.size());
	oCenter.z = oCenter.z / float(vPointIdx.size());

	//output
	return oCenter;

}

/*************************************************
Function: ComputeCenter - Reload
Description:  compute the center point of a given point set 
Input: vCloud - a 3D point clouds
*************************************************/
pcl::PointXYZ  Confidence::ComputeCenter(const PCLCloudXYZ & vCloud){

	//define output
	pcl::PointXYZ oCenter;
	oCenter.x = 0.0;
	oCenter.y = 0.0;
	oCenter.z = 0.0;

	//check whether the input has point 
	if (!vCloud.points.size())
		return oCenter;

	//calculate the average
	for (int i = 0; i != vCloud.points.size(); ++i) {
	
		oCenter.x = oCenter.x + vCloud.points[i].x;
		oCenter.y = oCenter.y + vCloud.points[i].y;
		oCenter.z = oCenter.z + vCloud.points[i].z;
	
	}

	oCenter.x = oCenter.x / float(vCloud.points.size());
	oCenter.y = oCenter.y / float(vCloud.points.size());
	oCenter.z = oCenter.z / float(vCloud.points.size());

	//output
	return oCenter;

}


/*************************************************
Function: GetRandom
Description: compute the Euclidean distance between two points
Calls: none
Called By: main function of project or other classes
Table Accessed: none
Table Updated: none
Input: oQueryPoint - the query point (based point)
oTargetPoint - the target point
Output: the distance value
Return: a distance value
Others: This function is the same with Compute1Norm, but it is a static one
*************************************************/
std::vector<int>  Confidence::GetRandom(const unsigned int iSize,
		                                  const int iSampleNums){

	//define output vector
	std::vector<int> vAllValueVec(iSize, 0);

	//get all of value
	for (int i = 0; i != iSize; ++i)
		vAllValueVec[i] = i;

	//if the sampling number is larger than the size of total number
	if (iSize <= iSampleNums) {

		return vAllValueVec;

	}

	//the last number
	int iLastIdx = iSize - 1;
	int iCurSampNum = 0;

	//get the random value
	while (iSampleNums - iCurSampNum) {

		//defend repeat selection
		int iRandomRes = (rand() % (iLastIdx - iCurSampNum + 1));
			
		//exchange the last one of unselected value and current randon selected value
		int iTempValue = vAllValueVec[iRandomRes];
		vAllValueVec[iRandomRes] = vAllValueVec[iLastIdx - iCurSampNum];
		vAllValueVec[iLastIdx - iCurSampNum] = iTempValue;
		//count
		iCurSampNum++;

	}//while

	//get the last iCurSampNum value of total value
	std::vector<int> iRandomVec;
	iRandomVec.reserve(iCurSampNum);
	for (int i = iLastIdx; i != (iLastIdx - iCurSampNum); --i) {

		iRandomVec.push_back(vAllValueVec[i]);

	}

	//over
	return iRandomVec;

}
//reload
/*//=================================================================================
std::vector<int> Confidence::GetRandom(const PCLCloudXYZPtr & pAllTravelCloud,
	                                                         GridMap & oMaper,
	                                                    const int iSampleNums){

	std::vector<int> vRandomVec;
	int iSize = pAllTravelCloud->points.size();
	//define output vector
	std::vector<int> vAllValueVec(pAllTravelCloud->points.size(), 0);

	//get all of value
	for (int i = 0; i != iSize; ++i)
		vAllValueVec[i] = i;

	//if the sampling number is larger than the size of total number
	if (iSize <= iSampleNums) {

		return vAllValueVec;

	}

	//the last number
	int iLastIdx = iSize - 1;
	int iCurSampNum = 0;

	//get the random value
	while (iSampleNums - iCurSampNum) {

		//defend repeat selection
		int iRandomRes = (rand() % (iLastIdx - iCurSampNum + 1));

		int iRandomGrid = oMaper.AssignPointToMap(pAllTravelCloud->points[iRandomRes]);

		if (oMaper.vConfidenceMap[iRandomGrid].label == 2) {
			//exchange the last one of unselected value and current randon selected value
			int iTempValue = vAllValueVec[iRandomRes];
			vAllValueVec[iRandomRes] = vAllValueVec[iLastIdx - iCurSampNum];
			vAllValueVec[iLastIdx - iCurSampNum] = iTempValue;
			//save the result
			vRandomVec.push_back(iRandomRes);
		//count
		iCurSampNum++;
        }

	}//while

	//over
	return vRandomVec;

}
=================================================================================8/


/*************************************************
Function: ComputeEuclideanDis
Description: compute the Euclidean distance between two points
Calls: none
Called By: main function of project or other classes
Table Accessed: none
Table Updated: none
Input: oQueryPoint - the query point (based point)
       oTargetPoint - the target point 
Output: the distance value
Return: a distance value
Others: This function is the same with Compute1Norm, but it is a static one
*************************************************/
float Confidence::ComputeEuclideanDis(pcl::PointXYZ & oQueryPoint, pcl::PointXYZ & oTargetPoint) {

	//compute the eucliden distance between the query point (robot) and target point (scanned point)
	return sqrt(pow(oQueryPoint.x - oTargetPoint.x, 2.0f)
		      + pow(oQueryPoint.y - oTargetPoint.y, 2.0f)
		      + pow(oQueryPoint.z - oTargetPoint.z, 2.0f));

}


/*************************************************
Function: DistanceTerm
Description: the function is to compute the distance feature to the confidence value
Calls: ComputeCenter
       GaussianKernel
Called By: main function of project 
Table Accessed: none
Table Updated: none
Input: vConfidenceMap - the confidence map (grid map)
	   oRobotPoint - the location of the robot  
	   vNearByIdxs - the neighboring grids based on the input robot location
	   vTravelCloud - the travelable point clouds (the ground point clouds)
	   vGridTravelPsIdx - the index of point within each grid to total travelable point clouds  
Output: the distance term value of each neighboring grid
Return: a vector saves distance value of each neighboring grid
Others: none
*************************************************/
void Confidence::DistanceTerm(std::vector<ConfidenceValue> & vConfidenceMap,
	                                      const pcl::PointXYZ & oRobotPoint,
                                   const std::vector<int> & vNearGroundIdxs,
	                                       const PCLCloudXYZ & vGroundCloud){

	//**compute the distance part** 
	for (int i = 0; i != vNearGroundIdxs.size(); ++i) {

		int iNearGridId = vNearGroundIdxs[i];
	
		//non-empty ground grid
		//if (vConfidenceMap[iNearGridId].label == 2) {

			//compute smooth distance using Gaussin Kernel based on the center point
			//the empty grid has zero value in this term
			float fGridTravelRes = GaussianKernel(oRobotPoint, 
				                                  vGroundCloud.points[i], 
				                                  m_fSigma);

			//**********Incremental item************
	        //fd(p) = max(fd(pi))  
		    if(vConfidenceMap[iNearGridId].travelTerm < fGridTravelRes)
		    	vConfidenceMap[iNearGridId].travelTerm = fGridTravelRes;


		//}//end if 

	}//end i
	
}


/*************************************************
Function: DistanceTerm
Description: the function is to compute the distance feature to the confidence value
Calls: ComputeCenter
       GaussianKernel
Called By: main function of project 
Table Accessed: none
Table Updated: none
Input: vConfidenceMap - the confidence map (grid map)
	   oRobotPoint - the location of the robot  
	   vNearByIdxs - the neighboring grids based on the input robot location
	   vTravelCloud - the travelable point clouds (the ground point clouds)
	   vGridTravelPsIdx - the index of point within each grid to total travelable point clouds  
Output: the distance term value of each neighboring grid
Return: a vector saves distance value of each neighboring grid
Others: none
*************************************************/
void Confidence::BoundTerm(std::vector<ConfidenceValue> & vConfidenceMap,
                               const std::vector<int> & vNearGroundIdxs,
	                                const PCLCloudXYZPtr & pGroundCloud,
	                                 const PCLCloudXYZPtr & pBoundCloud){

	 //**compute the center offset part**
	 //maybe there is not any boundary in an open area
	if (pBoundCloud->points.size()) {

        //define a threshold indicating where is dangerous for robot to close  
		float fNoTouchThr = (m_fSigma - 0.5) / m_fSigma;

		pcl::KdTreeFLANN<pcl::PointXYZ> oBoundTree;
		oBoundTree.setInputCloud(pBoundCloud);

		//for each non-empty neighboring grid
		for (int i = 0; i != vNearGroundIdxs.size(); ++i) {

			//compute the distance between boundary and travelable region
			std::vector<int> vSearchIdx;
			std::vector<float> vSearchDis;

			oBoundTree.nearestKSearch(pGroundCloud->points[i], 1, vSearchIdx, vSearchDis);

			//compute the boundary distance 
			float fBoundvalue = (m_fSigma - vSearchDis[0]) / m_fSigma;
			if (fBoundvalue < 0)
				fBoundvalue = 0.0;

			if (vConfidenceMap[vNearGroundIdxs[i]].boundTerm > fBoundvalue)
				vConfidenceMap[vNearGroundIdxs[i]].boundTerm = fBoundvalue;

            //also affect the travelable when the robot is too close to wall
			if (vConfidenceMap[vNearGroundIdxs[i]].boundTerm > fNoTouchThr)
                vConfidenceMap[vNearGroundIdxs[i]].travelable = 4;

		}//end i 

	}

}


/*************************************************
Function: OcclusionTerm
Description: the function is to compute the distance feature to the confidence value
Calls: ComputeCenter
GaussianKernel
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: vConfidenceMap - the confidence map (grid map)
oRobotPoint - the location of the robot
vNearByIdxs - the neighboring grids based on the input robot location
vTravelCloud - the travelable point clouds (the ground point clouds)
vGridTravelPsIdx - the index of point within each grid to total travelable point clouds
Output: the distance term value of each neighboring grid
Return: a vector saves distance value of each neighboring grid
Others: none
*************************************************/
void Confidence::OcclusionTerm(std::vector<ConfidenceValue> & vConfidenceMap,
	                                          PCLCloudXYZPtr & pNearAllCloud,
	                                const std::vector<int> & vNearGroundIdxs,
	                                    const pcl::PointXYZ & oPastViewPoint){ 

	//check the point cloud size (down sampling if point clouds is too large)
	unsigned int iNonGrndPSize = pNearAllCloud->points.size() - vNearGroundIdxs.size();
	unsigned int iSmplThr = 500000;

	int iSize = vNearGroundIdxs.size();

	int iiSize = pNearAllCloud->points.size();

    //if need sampling
	if(iNonGrndPSize > iSmplThr){

		pcl::PointCloud<pcl::PointXYZ> vSamplingClouds;
		//retain ground points
		for(int i = 0; i != vNearGroundIdxs.size(); ++i)
			vSamplingClouds.push_back(pNearAllCloud->points[i]);

		//down sampling
		int iSmplNum = int(iNonGrndPSize / iSmplThr);

		//only down sample the non-ground points 
		for(int i = vNearGroundIdxs.size(); i != pNearAllCloud->points.size(); i = i + iSmplNum){
			//sampling
			vSamplingClouds.push_back(pNearAllCloud->points[i]);

		}

        pNearAllCloud->clear();

		//restore the point clouds
		for(int i = 0; i != vSamplingClouds.points.size(); ++i)
			pNearAllCloud->points.push_back(vSamplingClouds.points[i]);//get back

	}//end if iNonGrndPSize > iSmplThr
   
	//using the GHPR algorithm 
	GHPR oGHPRer(4.2);

	//**********Measurement item************
	//compute the visibility based on the history of view points
	std::vector<int> vVisableIdx = oGHPRer.ComputeVisibility(*pNearAllCloud, oPastViewPoint);
	    
	//**********Incremental item************
	//fv(p) = fv(n)  
	for (int i = 0; i != vVisableIdx.size(); ++i){
		//if it is a ground point
		if(vVisableIdx[i] < vNearGroundIdxs.size())
			//get maximum value of occlusion term
			vConfidenceMap[vNearGroundIdxs[vVisableIdx[i]]].visiTerm += 1.0;

	}
   

}


/*************************************************
Function: QualityTermUsingDensity
Description: the function is to compute the distance feature to the confidence value
Calls: ComputeCenter
GaussianKernel
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: vConfidenceMap - the confidence map (grid map)
oRobotPoint - the location of the robot
vNearByIdxs - the neighboring grids based on the input robot location
vTravelCloud - the travelable point clouds (the ground point clouds)
vGridTravelPsIdx - the index of point within each grid to total travelable point clouds
Output: the distance term value of each neighboring grid
Return: a vector saves distance value of each neighboring grid
Others: none
*************************************************/
//void Confidence::QualityTermUsingDensity(std::vector<ConfidenceValue> & vConfidenceMap,
//                               const std::vector<int> & vNearByIdxs,
//	                                  const PCLCloudXYZ & vTravelCloud,
//	            const std::vector<std::vector<int>> & vGridTravelPsIdx,
//	                                const PCLCloudXYZ & vAllBoundCloud,
//	             const std::vector<std::vector<int>> & vGridBoundPsIdx,
//	                                const PCLCloudXYZ & vObstacleCloud,
//	               const std::vector<std::vector<int>> & vGridObsPsIdx) {
//
//	//save the point that is in an unreachable grid
//	for (int i = 0; i != vNearByIdxs.size(); ++i) {
//
//		//point clouds to be seen
//		PCLCloudXYZPtr pNearCloud(new PCLCloudXYZ);
//
//		int iOneGridIdx = vNearByIdxs[i];
//
//		//record the ground point
//		for (int j = 0; j != vGridTravelPsIdx[iOneGridIdx].size(); ++j)
//			pNearCloud->points.push_back(vTravelCloud.points[vGridTravelPsIdx[iOneGridIdx][j]]);
//
//		//record the boundary point
//		for (int j = 0; j != vGridBoundPsIdx[iOneGridIdx].size(); ++j)
//			pNearCloud->points.push_back(vAllBoundCloud.points[vGridBoundPsIdx[iOneGridIdx][j]]);
//
//		//record the obstacle point
//		for (int j = 0; j != vGridObsPsIdx[iOneGridIdx].size(); ++j)
//			pNearCloud->points.push_back(vObstacleCloud.points[vGridObsPsIdx[iOneGridIdx][j]]);
//		
//		//estimate the quality of feature
//		//compute the quality using the density feature
//		//vConfidenceMap[iOneGridIdx].quality = ComputeDensity(*pNearCloud,5);
//		//compute the quality using the standard deviation feature
//		vConfidenceMap[iOneGridIdx].quality = StandardDeviation(*pNearCloud);
//
//	}//end for i
//
//}

/*************************************************
Function: QualityTerm
Description: the function is to compute the distance feature to the confidence value
Calls: ComputeCenter
GaussianKernel
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: vConfidenceMap - the confidence map (grid map)
oRobotPoint - the location of the robot
vNearByIdxs - the neighboring grids based on the input robot location
vTravelCloud - the travelable point clouds (the ground point clouds)
vGridTravelPsIdx - the index of point within each grid to total travelable point clouds
Output: the distance term value of each neighboring grid
Return: a vector saves distance value of each neighboring grid
Others: none
*************************************************/
/*
void Confidence::QualityTerm(std::vector<ConfidenceValue> & vConfidenceMap,
	                          const std::vector<int> & vNearByIdxs,
	                               const PCLCloudXYZ & vAllBoundCloud,
	            const std::vector<std::vector<int>> & vGridBoundPsIdx,
	                               const PCLCloudXYZ & vObstacleCloud,
	              const std::vector<std::vector<int>> & vGridObsPsIdx){

	//point clouds to be seen
	PCLCloudXYZPtr pNearCloud(new PCLCloudXYZ);
	std::vector<int> vMeasuredGridIdx;

	//save the point that is in an unreachable grid
	for (int i = 0; i != vNearByIdxs.size(); ++i) {

		int iOneGridIdx = vNearByIdxs[i];

		//if this grid is a obstacle grid or boundary grid
		if(vConfidenceMap[iOneGridIdx].label == 1
		   || vConfidenceMap[iOneGridIdx].label == 3){
			
			//record this grid idx
			vMeasuredGridIdx.push_back(iOneGridIdx);

			//record the boundary point
			//for (int j = 0; j != vGridBoundPsIdx[iOneGridIdx].size(); ++j)
			//	pNearCloud->points.push_back(vAllBoundCloud.points[vGridBoundPsIdx[iOneGridIdx][j]]);
			
			//record the obstacle point
			for (int j = 0; j != vGridObsPsIdx[iOneGridIdx].size(); ++j)
				pNearCloud->points.push_back(vObstacleCloud.points[vGridObsPsIdx[iOneGridIdx][j]]);
			
			//estimate the quality using Hausdorff measure based method
	
		}//end if

	}//end for i

	//using Hausdorff Dimension to measure point clouds
	HausdorffDimension oHDor(5, 1);
	//set the 
	oHDor.SetMinDis(0.1);
	//set the dimension type
	oHDor.SetParaQ(0);
	//compute the Hausdorff result

	float fHausRes = oHDor.BoxCounting(*pNearCloud);
    
	//assigment
	for (int i = 0; i != vMeasuredGridIdx.size(); ++i) {
	
		vConfidenceMap[vMeasuredGridIdx[i]].quality = fabs(fHausRes - 2.0f);

	}

}//0412*/



/*************************************************
Function: BoundaryTerm
Description: the function is to compute the distance feature to the confidence value
Calls: ComputeCenter
       GaussianKernel
Called By: main function of project 
Table Accessed: none
Table Updated: none
Input: vConfidenceMap - the confidence map (grid map)
	   oRobotPoint - the location of the robot  
	   vNearByIdxs - the neighboring grids based on the input robot location
	   vTravelCloud - the travelable point clouds (the ground point clouds)
	   vGridTravelPsIdx - the index of point within each grid to total travelable point clouds  
Output: the distance term value of each neighboring grid
Return: a vector saves distance value of each neighboring grid
Others: none
*************************************************/
//std::vector<float> Confidence::BoundaryTerm(PCLCloudXYZ & vTravelCloud, PCLCloudXYZ & vBoundCloud, pcl::PointXYZ & oRobotPoint){
//	
//	//new output
//	std::vector<float> vBoundRes;
//	if (!vBoundCloud.points.size()) {
//		vBoundRes.resize(vTravelCloud.points.size(), ROBOT_AFFECTDIS);
//		return vBoundRes;
//	}
//	std::cout << "bound points: " << vBoundCloud.points.size() << std::endl;
//	//preparation
//	pcl::PointCloud<pcl::PointXYZ>::Ptr pBoundKDCloud(new pcl::PointCloud<pcl::PointXYZ>);
//	for (int i = 0; i != vBoundCloud.points.size(); ++i)
//		pBoundKDCloud->points.push_back(vBoundCloud.points[i]);
//
//	//new a kdtree for query point clouds
//	pcl::KdTreeFLANN<pcl::PointXYZ> oBoundKDTree;
//	oBoundKDTree.setInputCloud(pBoundKDCloud);
//
//	//
//	for (int i = 0; i < vTravelCloud.points.size(); ++i) {
//
//		//
//		std::vector<int> vSearchedIdx;
//		std::vector<float> vSearchedDis;
//		//
//		oBoundKDTree.nearestKSearch(vTravelCloud.points[i], 1, vSearchedIdx, vSearchedDis);
//		//
//		//if (vSearchedDis[0] > 5.0)
//		//	vSearchedDis[0] = 5.0;
//
//		vBoundRes.push_back(vSearchedDis[0]);
//
//	}//end i
//
//	return vBoundRes;
//
//}


/*************************************************
Function: FrontierTerm
Description: the function is to compute the frontier feature, which is popular in roboticmethod
Calls: none
       none
Called By: main function of project 
Table Accessed: none
Table Updated: none
Input: vConfidenceMap - the confidence map (grid map)
	   iQueryGrid - the grid at which the robot right now is
	   vNearByIdxs - the neighboring grids index based on the robot grid
Output: change the confidence value about frontier part
Return: none
Others: none
*************************************************/
//void Confidence::FrontierTerm(std::vector<ConfidenceValue> & vConfidenceMap, 
//	                                            const int & iQueryGrid,
//	                           const std::vector<int> & vNearByIdxs){
//
//	//variables
//	float fBoundaryRes = 0.0;
//	int iUnkownCount = 0;
//
//	//if it is a ground region
//	if (vConfidenceMap[iQueryGrid].label == 2) {
//
//		for (int k = 0; k != vNearByIdxs.size(); ++k) {
//			//count its neighboring unknown grids
//			if (!vConfidenceMap[vNearByIdxs[k]].bKnownFlag) {
//				iUnkownCount++;
//			}
//
//		}//end for k
//
//	}//end if vConfidenceMap
//
//	//it has a high value if it is far away from the boundary 
//	if (!iUnkownCount)
//		fBoundaryRes = 1.0;
//
//	vConfidenceMap[iQueryGrid].boundary = fBoundaryRes;
//
//}



/*************************************************
Function: ComputeTotalCoffidence
Description: the function is to compute the distance feature to the confidence value
Calls: ComputeCenter
       GaussianKernel
Called By: main function of project 
Table Accessed: none
Table Updated: none
Input: vConfidenceMap - the confidence map (grid map)
	   oRobotPoint - the location of the robot  
	   vNearByIdxs - the neighboring grids based on the input robot location
	   vTravelCloud - the travelable point clouds (the ground point clouds)
	   vGridTravelPsIdx - the index of point within each grid to total travelable point clouds  
Output: the distance term value of each neighboring grid
Return: a vector saves distance value of each neighboring grid
Others: none
*************************************************/
void Confidence::ComputeTotalCoffidence(std::vector<ConfidenceValue> & vConfidenceMap, 
	                                                           const int & iQueryIdx){

    //get maximum value of distance term
    //float fTotalVal =  m_fLenWeight * vDisPartValue[i] 
    //                + m_fBoundWeight * vBoundPartValue[i] 
    //                + m_fWeightVis * LinearKernel(vConfidenceMap[iQueryIdx].visibility, m_fVisTermThr);

    float fTotalVal =  m_fLenWeight * vConfidenceMap[iQueryIdx].travelTerm 
                     + m_fBoundWeight * vConfidenceMap[iQueryIdx].boundTerm;
			
	if (vConfidenceMap[iQueryIdx].totalValue < fTotalVal)
		vConfidenceMap[iQueryIdx].totalValue = fTotalVal;

}


/*************************************************
Function: RegionGrow
Description: this function is to find the reachable grid based on current robot location
Calls: none
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: vNewScannedGrids - the neighboring grid of the current robot location
Output: the reachable label of grid map. The grid labelled as 1 is reachable grid 
Return: none
Others: point with label 2 is queried and changed one by one during implementing function
        point with label 0 will becomes point with label 2 after implementing function, this is to prepare next computing
*************************************************/
void Confidence::RegionGrow(std::vector<ConfidenceValue> & vConfidenceMap,
	                            const std::vector<int> & vNewScannedGrids,
								        const ExtendedGM & oExtendGridMap){

    //status of grid in region grow
	//-1 indicates it is an unknown grid
	//0 indicates this grid is ground but not reachable now
    //1 indicates this grid is a travelable region grid
	//2 is the new scanned grids (input) without growing calculation (in this time)
	//3 indicates the grid has been computed
	//4 indicates this grid is a off groud grid (not reachable forever)
	
	for(int i = 0; i != vNewScannedGrids.size(); ++i){
		//if it is unknown (never be computed before)
		if(vConfidenceMap[vNewScannedGrids[i]].travelable == -1){
			//if it is a ground grid
			if(vConfidenceMap[vNewScannedGrids[i]].label == 2)
		       vConfidenceMap[vNewScannedGrids[i]].travelable = 2;
			else
			   vConfidenceMap[vNewScannedGrids[i]].travelable = 4;//off-ground
		}
	}

	//record computed grid (record which grid has been computed with a status value)
	std::vector<int> vComputedGridIdx;

    //check each input grid
	for(int i = 0; i != vNewScannedGrids.size(); ++i){

		//current seed index
	    int iCurIdx;
		//inital flag as 3
		int bTravelableFlag = 3;
		
		//seeds
	    std::vector<int> vSeeds;  
		std::vector<int> vSeedHistory;
		//get one input grid as seed
		vSeeds.push_back(vNewScannedGrids[i]);

	    //if this one has not been grown
		if(vConfidenceMap[vNewScannedGrids[i]].travelable == 2){
            //growing
	        while(!vSeeds.empty()){

		        //choose a seed (last one)
		        iCurIdx = *(vSeeds.end()-1);
		        //delete the last one
		        vSeeds.pop_back();

			    //record the history of seeds
			    vSeedHistory.push_back(iCurIdx);
			    vComputedGridIdx.push_back(iCurIdx);
			    //label this seed as it has being considered
				vConfidenceMap[iCurIdx].travelable = 3;

				//find the nearboring grids
				//std::vector<int> vNearGridIdx = SearchGrids(iCurIdx, 0.5);
				std::vector<int> vNearGridIdx;
				ExtendedGM::CircleNeighborhood(vNearGridIdx,
						                       oExtendGridMap.m_oFeatureMap, 
											   oExtendGridMap.m_vGrowSearchMask,
		                                       iCurIdx);

				//check neighboring grids
				for (int i = 0; i != vNearGridIdx.size(); ++i) {
					//if the near grid is new input
					if (vConfidenceMap[vNearGridIdx[i]].travelable == 2)
						vSeeds.push_back(vNearGridIdx[i]);

					//if the near grid is reachable so that the query ground grids must be also reachable
					if (vConfidenceMap[vNearGridIdx[i]].travelable == 1)
						bTravelableFlag = 2;
				}//end for int i = 0;i!=vNearGridIdx.size();++i
				 
		    }//end while

		    //assignment as a touchable grid or ioslated grids
			for(int i = 0; i != vSeedHistory.size(); ++i){
			        vConfidenceMap[vSeedHistory[i]].travelable -= bTravelableFlag;
			}

	    }//end if vConfidenceMap[vNearGridIdx[i]].travelable == 2
	
	}//end for

	//search the unreachable grid
	//the reachable region is with respect to current query location
	//because an unreachable grid may be reachable if the robot moves close
	for (int i = 0; i != vComputedGridIdx.size(); ++i) {
	    //prepare for further growing
		if(vConfidenceMap[vComputedGridIdx[i]].travelable == 0)
		   vConfidenceMap[vComputedGridIdx[i]].travelable = 2;
	}

}


/*************************************************
Function: Normalization
Description: normalize the input feature vector
Calls: none
Called By: major function
Table Accessed: none
Table Updated: none
Input: vFeatures - a feature (feature value of each grid or feature value of each point)
Output: change the feature value and limit it between 0 and 1
Return: none
Others: 0 <= vFeatures[i] <= 1
*************************************************/
void Confidence::Normalization(std::vector<float> & vFeatures){

	//maximam and minimam
	float fMaxValue = -FLT_MAX;
	float fMinValue = FLT_MAX;

	//find the maximun and minmum value
	for (int i = 0; i != vFeatures.size(); ++i) {
	
		if (vFeatures[i] > fMaxValue)
			fMaxValue = vFeatures[i];
		if (vFeatures[i] < fMinValue)
			fMinValue = vFeatures[i];
	}

	//Normalization
	for (int i = 0; i != vFeatures.size(); ++i) 
		vFeatures[i] = (vFeatures[i] - fMinValue) / (fMaxValue - fMinValue);
	
}


}/*namespace*/
