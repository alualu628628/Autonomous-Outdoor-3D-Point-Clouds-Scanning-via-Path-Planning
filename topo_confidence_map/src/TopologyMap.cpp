#include "TopologyMap.h"


namespace topology_map {


//*********************************Initialization function*********************************
/*************************************************
Function: TopologyMap
Description: constrcution function for TopologyMap class
Calls: all member functions
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: global node,
       privare node
	   flag of generating output file
	   original frame value
	   Output: none
Return: none
Others: the HandlePointClouds is the kernel function
*************************************************/
TopologyMap::TopologyMap(ros::NodeHandle & node,
	                     ros::NodeHandle & nodeHandle):
                         m_oCnfdnSolver(12.0,4.2,5,0.6),
	                     m_pBoundCloud(new pcl::PointCloud<pcl::PointXYZ>),
	                     m_pObstacleCloud(new pcl::PointCloud<pcl::PointXYZ>),
	                     m_iTrajFrameNum(0),
	                     m_iGroundFrames(0),
	                     m_iBoundFrames(0),
	                     m_iObstacleFrames(0),
	                     m_iComputedFrame(0),
	                     m_iNodeTimes(0),
	                     m_iOdomSampingNum(25),
	                     m_bGridMapReadyFlag(false),
	                     m_bCoverFileFlag(false),
	                     m_bOutTrajFileFlag(false){

	//read parameters
	ReadTopicParams(nodeHandle);

	//subscribe topic 
	//m_oOctoMapClient = nodeHandle.serviceClient<octomap_msgs::GetOctomap>(m_oOctomapServiceTopic);

	//subscribe (hear) the odometry information
	m_oOdomSuber = nodeHandle.subscribe(m_sOdomTopic, 1, &TopologyMap::HandleTrajectory, this);

	//subscribe (hear) the point cloud topic with different classifications from ground extratcion 
	m_oGroundSuber = nodeHandle.subscribe(m_sGroundTopic, 1, &TopologyMap::HandleGroundClouds, this);
	m_oBoundSuber = nodeHandle.subscribe(m_sBoundTopic, 1, &TopologyMap::HandleBoundClouds, this);
	m_oObstacleSuber = nodeHandle.subscribe(m_sObstacleTopic, 1, &TopologyMap::HandleObstacleClouds, this);

	//publish topic
	m_oGridMapPublisher = nodeHandle.advertise<grid_map_msgs::GridMap>("grid_map", 1, true);

	m_oCloudPublisher = nodeHandle.advertise<sensor_msgs::PointCloud2>("test_clouds", 1, true);

	m_oGoalPublisher = nodeHandle.advertise<nav_msgs::Odometry>("goal_odom", 1, true);

}

/*************************************************
Function: TopologyMap
Description: constrcution function for TopologyMap class
Calls: all member functions
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: global node,
		   privare node
		   flag of generating output file
		   original frame value
	Output: none
	Return: none
	Others: the HandlePointClouds is the kernel function
*************************************************/

TopologyMap::~TopologyMap() {

}

	/*************************************************
	Function: TopologyMap
	Description: constrcution function for TopologyMap class
	Calls: all member functions
	Called By: main function of project
	Table Accessed: none
	Table Updated: none
	Input: global node,
	privare node
	flag of generating output file
	original frame value
	Output: none
	Return: none
	Others: the HandlePointClouds is the kernel function
	*************************************************/

bool TopologyMap::ReadTopicParams(ros::NodeHandle & nodeHandle) {

	//output file name
	nodeHandle.param("file_outputpath", m_sFileHead, std::string("./"));

	//input topic
	nodeHandle.param("odom_in_topic", m_sOdomTopic, std::string("/odometry/filtered"));

	nodeHandle.param("ground_pc_topic", m_sGroundTopic, std::string("/ground_points"));

	nodeHandle.param("boundary_pc_topic", m_sBoundTopic, std::string("/boundary_points"));

	nodeHandle.param("obstacle_pc_topic", m_sObstacleTopic, std::string("/obstacle_points"));

	//intput service
	//nodeHandle.param("octomap_service_topic", m_oOctomapServiceTopic, std::string("/octomap_binary"));

	//frequncy
	nodeHandle.param("odometry_rawfreq", m_dOdomRawHz, 50.0);

	nodeHandle.param("odomsampling_freq", m_dSamplingHz, 2.0);

	m_iOdomSampingNum = int(m_dOdomRawHz / m_dSamplingHz);
	ROS_INFO("Set odometry down sampling times as [%d]", m_iOdomSampingNum);

	//
	//nodeHandle.param("freshoctomap_freq", m_dOctomapFreshHz, 0.5);

	//m_iMapFreshNum = int(m_dOdomRawHz / m_dOctomapFreshHz); 
	//ROS_INFO("Set updating octomap times as [%d]",m_iMapFreshNum);
    double dPastViewDuration;
	nodeHandle.param("pastview_duration", dPastViewDuration, 5.0);
	m_iPastOdomNum = int(dPastViewDuration * m_dSamplingHz);
	if(m_iPastOdomNum <= 0)
	   m_iPastOdomNum = 1;//at least one point (in this case, this point will be front and back point of quene)

	double dShockDuration;
	nodeHandle.param("shock_duration", dShockDuration, 8.0);
	m_iShockNum = int(dShockDuration * m_dSamplingHz);
	if(m_iShockNum <= 0)
	   m_iShockNum = 100;//at least one point (in this case, this point will be front and back point of quene)


    double dViewZOffset;
	nodeHandle.param("pastview_zoffset", dViewZOffset, 0.0);
	m_fViewZOffset = float(dViewZOffset);

	//point cloud sampling number
	nodeHandle.param("pointframe_smplnum", m_iPCSmplNum, 1);

	//***************grid map parameters***************
	//map range/map size
	double dMapMaxRange;
	nodeHandle.param("gridmap_maxrange", dMapMaxRange, 250.0);
	if (dMapMaxRange <= 0)//defend a zero input
		dMapMaxRange = 50.0;
	
	//map cell size
	double dResolution;
	nodeHandle.param("gridmap_resolution", dResolution, 0.1);
	if (dResolution <= 0)
		dResolution = 0.1;

	//map limited height
	double dMinMapZ;
	nodeHandle.param("min_mapz", dMinMapZ, -2.0);
	double dMaxMapZ;
	nodeHandle.param("max_mapz", dMaxMapZ, 7.0);

	//map id
	std::string sMapFrameID;
	nodeHandle.param("gridmap_frameid", sMapFrameID, std::string("map"));

    //generate grid map
	m_oGMer.GetParam(dMapMaxRange,
		              dResolution,
		                 dMinMapZ,
		                 dMaxMapZ,
		              sMapFrameID);

	//robot's neighborhood searching radius
	double dRbtLocalRadius;
	nodeHandle.param("robot_local_r", dRbtLocalRadius, 5.0);
	m_oCnfdnSolver.SetSigmaValue(dRbtLocalRadius);

	//generate robot neighborhood searching mask 
	m_oGMer.m_vRobotSearchMask.clear();
	m_oGMer.m_vRobotSearchMask = m_oGMer.GenerateCircleMask(dRbtLocalRadius);

	//node neighborhood searching radius
	double dNodeRadiusRate;
	nodeHandle.param("nodegenerate_rate", dNodeRadiusRate, 0.5);

	//generate node generation neighborhood mask
	double dNodeRadius =  dRbtLocalRadius * dNodeRadiusRate;
	m_oGMer.m_vNodeMadeMask.clear();
	m_oGMer.m_vNodeMadeMask = m_oGMer.GenerateCircleMask(dNodeRadius);

	//generate region grow neighborhood mask
	//eight unicom
	double dRegionGrowR;
	nodeHandle.param("regiongrow_r", dRegionGrowR, 0.5);
	m_oGMer.m_vGrowSearchMask.clear();
	m_oGMer.m_vGrowSearchMask = m_oGMer.GenerateCircleMask(dRegionGrowR);

	m_oGMer.m_vBoundDefendMask.clear();
    m_oGMer.m_vBoundDefendMask = m_oGMer.GenerateCircleMask(1.5*dRegionGrowR);

	double dInitialR;
	nodeHandle.param("initial_r", dInitialR, 4.5);
    m_oGMer.m_vInitialMask.clear();
	m_oGMer.m_vInitialMask = m_oGMer.GenerateCircleMask(dInitialR);
 
	m_oGMer.m_vLocalQualityMask.clear();//the local region of dimension based method
	m_oGMer.m_vLocalQualityMask = m_oGMer.GenerateCircleMask(1.0);

	//about confidence feature weight
	double dTraversWeight;
	nodeHandle.param("travers_weight", dTraversWeight, 0.9);
	float fTraversWeight = float(dTraversWeight);

	double dDisWeight;
	nodeHandle.param("traversdis_weight", dDisWeight, 0.6);
	float fDisWeight = float(dDisWeight);

	m_oCnfdnSolver.SetTermWeight(fTraversWeight, fDisWeight);

	return true;

}

/*************************************************
Function: TopologyMap
	Description: constrcution function for TopologyMap class
	Calls: all member functions
	Called By: main function of project
	Table Accessed: none
	Table Updated: none
	Input: global node,
		   privare node
		   flag of generating output file
		   original frame value
	Output: none
	Return: none
	Others: the HandlePointClouds is the kernel function
	*************************************************/

void TopologyMap::InitializeGridMap(const pcl::PointXYZ & oRobotPos) {

	//generate grid map
    m_oGMer.GenerateMap(oRobotPos);

	//get grid number in cols
	ExtendedGM::iGridRawNum = m_oGMer.m_oFeatureMap.getSize()(1);
	ROS_INFO("Set grid raw number as [%d]", ExtendedGM::iGridRawNum);

	//generate confidence map
	m_vConfidenceMap.clear();

    ConfidenceValue oGridCnfd;
	for(int i = 0; i != m_oGMer.m_oFeatureMap.getSize()(0)*m_oGMer.m_oFeatureMap.getSize()(1); ++i)
	    m_vConfidenceMap.push_back(oGridCnfd);


	m_vBoundPntMapIdx.resize(m_oGMer.m_oFeatureMap.getSize()(0)*m_oGMer.m_oFeatureMap.getSize()(1));
	m_vObstlPntMapIdx.resize(m_oGMer.m_oFeatureMap.getSize()(0)*m_oGMer.m_oFeatureMap.getSize()(1));

	//initial elevation map and center point clouds
	for (int i = 0; i != m_oGMer.m_oFeatureMap.getSize()(0); ++i) {//i

		for (int j = 0; j != m_oGMer.m_oFeatureMap.getSize()(1); ++j) {//j

			grid_map::Index oGridIdx;
			oGridIdx(0) = i;
			oGridIdx(1) = j;
			int iGridIdx = ExtendedGM::TwotoOneDIdx(oGridIdx);

			//find the 1d and 2d index
			grid_map::Position oGridPos;
			m_oGMer.m_oFeatureMap.getPosition(oGridIdx, oGridPos);
			//center point
			m_vConfidenceMap[iGridIdx].oCenterPoint.x = oGridPos.x();
			m_vConfidenceMap[iGridIdx].oCenterPoint.y = oGridPos.y();
		}//end i

	}//end j

    //get the neighborhood of original coordiante value and initial a rough travelable region
	std::vector<MapIndex> vOriginalNearIdx;
	ExtendedGM::CircleNeighborhood(vOriginalNearIdx,
	                               m_oGMer.m_oFeatureMap,
	                               m_oGMer.m_vInitialMask,
	                               oRobotPos);

    //only assign the travelable value,which means the node will be created only on the grid that has the actual data  
    for(int i = 0; i != vOriginalNearIdx.size(); ++i){
    	m_vConfidenceMap[vOriginalNearIdx[i].iOneIdx].travelable = 1;
    	m_vConfidenceMap[vOriginalNearIdx[i].iOneIdx].nodeCount = m_iNodeTimes;
    }

    //initial op solver
    m_oOPSolver.Initial(oRobotPos, m_oGMer.m_oFeatureMap);

    //initial astar map
    m_oAstar.InitAstarTravelMap(m_oGMer.m_oFeatureMap);

	m_bGridMapReadyFlag = true;

}




//*********************************Traversing / retrieving function*********************************



/*************************************************
Function: DevidePointClouds
Description: constrcution function for TopologyMap class
Calls: all member functions
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: global node,
privare nodem_dMapMaxRange
flag of generating output file
original frame value
Output: none
Return: none
Others: the HandlePointClouds is the kernel function
*************************************************/
void TopologyMap::DevidePointClouds(pcl::PointCloud<pcl::PointXYZ> & vNearGrndClouds,
	                                          std::vector<int> & vNearGroundGridIdxs,
                                   pcl::PointCloud<pcl::PointXYZ> & vNearBndryClouds,
                                    pcl::PointCloud<pcl::PointXYZ> & vNearObstClouds,	                                          
                                           const std::vector<MapIndex> & vNearByIdxs,
                                                               const int & iNodeTime){

	//prepare and clear
	vNearGrndClouds.clear();
    vNearBndryClouds.clear();
    vNearObstClouds.clear();
	vNearGroundGridIdxs.clear();

	//to each nearby grids
	for (int i = 0; i != vNearByIdxs.size(); ++i) {

		int iNearGridId = vNearByIdxs[i].iOneIdx;
		//assign to cooresponding point clouds based on its label
        switch (m_vConfidenceMap[iNearGridId].label){

        	case 1 : //the grid is a obstacle grid
        	    for (int j = 0; j != m_vObstlPntMapIdx[iNearGridId].size(); ++j){
        	    	if(m_vObstNodeTimes[m_vObstlPntMapIdx[iNearGridId][j]] == iNodeTime)//if it is recorded at current node time
        	    		vNearObstClouds.points.push_back(m_pObstacleCloud->points[m_vObstlPntMapIdx[iNearGridId][j]]);
        	    }//end for
            break;

            case 2 : //the grid is a ground grid
                vNearGrndClouds.points.push_back(m_vConfidenceMap[iNearGridId].oCenterPoint);
        	    vNearGroundGridIdxs.push_back(iNearGridId);
        	    //if the obstacle is large (perhaps some obstacles above the ground,e.g.,leafs points, high vegetation)
        	    if(m_vObstlPntMapIdx[iNearGridId].size() > 20){
        	    	for (int j = 0; j != m_vObstlPntMapIdx[iNearGridId].size(); ++j)
        	    		vNearObstClouds.points.push_back(m_pObstacleCloud->points[m_vObstlPntMapIdx[iNearGridId][j]]);
        	    }
            break;

            case 3 : //the grid is a boundary grid
                for (int j = 0; j != m_vBoundPntMapIdx[iNearGridId].size(); ++j)
				    vNearBndryClouds.points.push_back(m_pBoundCloud->points[m_vBoundPntMapIdx[iNearGridId][j]]);
				for (int j = 0; j != m_vObstlPntMapIdx[iNearGridId].size(); ++j){
					if(m_vObstNodeTimes[m_vObstlPntMapIdx[iNearGridId][j]] == iNodeTime)//if it is recorded at current node time
						vNearObstClouds.points.push_back(m_pObstacleCloud->points[m_vObstlPntMapIdx[iNearGridId][j]]);
        	    }//end for j
            break;

            default:
            break;
        }//end switch 

    }//end for i

}
//reload without label input
void TopologyMap::DevidePointClouds(pcl::PointCloud<pcl::PointXYZ> & vNearGrndClouds,
                                  pcl::PointCloud<pcl::PointXYZ> & vNearBndryClouds,
                                    pcl::PointCloud<pcl::PointXYZ> & vNearAllClouds,
	                                         std::vector<int> & vNearGroundGridIdxs,
                                          const std::vector<MapIndex> & vNearByIdxs){

	//prepare and clear
	vNearGrndClouds.clear();
    vNearBndryClouds.clear();
    vNearAllClouds.clear();
	vNearGroundGridIdxs.clear();

	pcl::PointCloud<pcl::PointXYZ> vNearObstClouds;//nearby obstacle point clouds

	//to each nearby grids
	for (int i = 0; i != vNearByIdxs.size(); ++i) {

		int iNearGridId = vNearByIdxs[i].iOneIdx;
		//assign to cooresponding point clouds based on its label
        switch (m_vConfidenceMap[iNearGridId].label){

        	case 1 : //the grid is a obstacle grid
        	    for (int j = 0; j != m_vObstlPntMapIdx[iNearGridId].size(); ++j)
        	    	vNearObstClouds.points.push_back(m_pObstacleCloud->points[m_vObstlPntMapIdx[iNearGridId][j]]);
            break;

            case 2 : //the grid is a ground grid
                vNearGrndClouds.points.push_back(m_vConfidenceMap[iNearGridId].oCenterPoint);
        	    vNearGroundGridIdxs.push_back(iNearGridId);
        	    //if the obstacle is large (perhaps some obstacles above the ground,e.g.,leafs points, high vegetation)
        	    if(m_vObstlPntMapIdx[iNearGridId].size() > 20){
        	    	for (int j = 0; j != m_vObstlPntMapIdx[iNearGridId].size(); ++j)
        	    		vNearObstClouds.points.push_back(m_pObstacleCloud->points[m_vObstlPntMapIdx[iNearGridId][j]]);
        	    }
            break;

            case 3 : //the grid is a boundary grid
                for (int j = 0; j != m_vBoundPntMapIdx[iNearGridId].size(); ++j)
				    vNearBndryClouds.points.push_back(m_pBoundCloud->points[m_vBoundPntMapIdx[iNearGridId][j]]);
				for (int j = 0; j != m_vObstlPntMapIdx[iNearGridId].size(); ++j)
        	    	vNearObstClouds.points.push_back(m_pObstacleCloud->points[m_vObstlPntMapIdx[iNearGridId][j]]);
            break;

            default:
            break;
        }

    }

    //make a all label point clouds (for occlusion detection)
    vNearAllClouds.reserve(vNearGrndClouds.size() + vNearBndryClouds.size() + vNearObstClouds.size());

    for(int i = 0; i != vNearGrndClouds.size(); ++i)
    	vNearAllClouds.push_back(vNearGrndClouds.points[i]);

    for(int i = 0; i != vNearBndryClouds.size(); ++i)
    	vNearAllClouds.push_back(vNearBndryClouds.points[i]);

    for(int i = 0; i != vNearObstClouds.size(); ++i)
    	vNearAllClouds.push_back(vNearObstClouds.points[i]);

}
//reload without generating vNearAllClouds and label
void TopologyMap::DevidePointClouds(pcl::PointCloud<pcl::PointXYZ> & vNearGrndClouds,
                                  pcl::PointCloud<pcl::PointXYZ> & vNearBndryClouds,
	                                         std::vector<int> & vNearGroundGridIdxs,
                                          const std::vector<MapIndex> & vNearByIdxs){

	//prepare and clear
	vNearGrndClouds.clear();
    vNearBndryClouds.clear();
	vNearGroundGridIdxs.clear();

	pcl::PointCloud<pcl::PointXYZ> vNearObstClouds;//nearby obstacle point clouds

	//to each nearby grids
	for (int i = 0; i != vNearByIdxs.size(); ++i) {

		int iNearGridId = vNearByIdxs[i].iOneIdx;
		//assign to cooresponding point clouds based on its label
        switch (m_vConfidenceMap[iNearGridId].label){

        	case 1 : //the grid is a obstacle grid
        	    for (int j = 0; j != m_vObstlPntMapIdx[iNearGridId].size(); ++j)
        	    	vNearObstClouds.points.push_back(m_pObstacleCloud->points[m_vObstlPntMapIdx[iNearGridId][j]]);
            break;

            case 2 : //the grid is a ground grid
                vNearGrndClouds.points.push_back(m_vConfidenceMap[iNearGridId].oCenterPoint);
        	    vNearGroundGridIdxs.push_back(iNearGridId);
            break;

            default:
            break;
        }//end switch

    }//end for int

}

//*********************************Processing function*********************************
/*************************************************
Function: HandleTrajectory
Description: a callback function in below:
m_oOdomSuber = node.subscribe(m_sOdomTopic, 5, &GroundExtraction::HandleTrajectory, this);
Calls: none
Called By: TransformLaserInOdom, which is the construction function
Table Accessed: none
Table Updated: none
Input: rawpoint, a 3d point with pcl point type
Output: a point clouds are almost the same with raw point clouds but timestamp values
Return: none
Others: none
*************************************************/
void TopologyMap::SamplingPointClouds(pcl::PointCloud<pcl::PointXYZ>::Ptr & pCloud,
	                                 std::vector<std::vector<int> > & vPointMapIdx,
	                                                                  int iSmplNum){

    //point clouds sampling based on the idx vector
	for(int i = 0; i != vPointMapIdx.size(); ++i){
		//retain at least one point in grid
		if(vPointMapIdx[i].size()){

			std::vector<int> vOneSmplIdx;
		        //sampling
			for(int j = 0; j != vPointMapIdx[i].size(); j = j + iSmplNum)
				vOneSmplIdx.push_back(vPointMapIdx[i][j]);
			

            vPointMapIdx[i].clear();
            for(int j = 0; j != vOneSmplIdx.size(); ++j)
                vPointMapIdx[i].push_back(vOneSmplIdx[j]);

        }//end if

    }//end for


    pcl::PointCloud<pcl::PointXYZ> vSmplClouds;

    //save each corresponding points
    for(int i = 0; i != vPointMapIdx.size(); ++i){
    	for(int j = 0; j != vPointMapIdx[i].size(); ++j){
			vSmplClouds.push_back(pCloud->points[vPointMapIdx[i][j]]);	
        }
	}

    //clear the point vector and save the new sampled points
	pCloud->points.clear();
	for(int i = 0; i != vSmplClouds.size(); ++i)
		pCloud->points.push_back(vSmplClouds.points[i]);


}
//reload sampling with point number
void TopologyMap::SamplingPointClouds(pcl::PointCloud<pcl::PointXYZ>::Ptr & pCloud,
	                                 std::vector<std::vector<int> > & vPointMapIdx,
	                                               std::vector<int> & vCloudLabels,
	                                                                  int iSmplNum){



    //point clouds sampling based on the idx vector
	for(int i = 0; i != vPointMapIdx.size(); ++i){
		//retain at least one point in grid
		if(vPointMapIdx[i].size()){

			std::vector<int> vOneSmplIdx;
		    //sampling
			for(int j = 0; j != vPointMapIdx[i].size(); j = j + iSmplNum)
				vOneSmplIdx.push_back(vPointMapIdx[i][j]);
			
            //clear row data and the turn other rows
            vPointMapIdx[i].clear();
            for(int j = 0; j != vOneSmplIdx.size(); ++j)
                vPointMapIdx[i].push_back(vOneSmplIdx[j]);

        }//end if

    }//end for


    pcl::PointCloud<pcl::PointXYZ> vSmplClouds;
    std::vector<int> vSmpCLoudLabels;

    //save each corresponding points
    for(int i = 0; i != vPointMapIdx.size(); ++i){
    	for(int j = 0; j != vPointMapIdx[i].size(); ++j){
			vSmplClouds.push_back(pCloud->points[vPointMapIdx[i][j]]);	
			vSmpCLoudLabels.push_back(vCloudLabels[vPointMapIdx[i][j]]);
        }
	}

    //clear the point vector and save the new sampled points
	pCloud->points.clear();
	vCloudLabels.clear();
	for(int i = 0; i != vSmplClouds.size(); ++i){
		pCloud->points.push_back(vSmplClouds.points[i]);
		vCloudLabels.push_back(vSmpCLoudLabels[i]);
	}


}

/*************************************************
Function: HandleTrajectory
Description: a callback function in below:
m_oOdomSuber = node.subscribe(m_sOdomTopic, 5, &GroundExtraction::HandleTrajectory, this);
Calls: none
Called By: TransformLaserInOdom, which is the construction function
Table Accessed: none
Table Updated: none
Input: rawpoint, a 3d point with pcl point type
Output: a point clouds are almost the same with raw point clouds but timestamp values
Return: none
Others: none
*************************************************/
void TopologyMap::HandleTrajectory(const nav_msgs::Odometry & oTrajectory) {

	//bool bMapUpdateFlag = false;
	bool bSamplingFlag = false;

	if (!m_bGridMapReadyFlag){
		//initial the grid map based on the odometry center

		pcl::PointXYZ oOdomPoint;
		oOdomPoint.x = oTrajectory.pose.pose.position.x;//z in loam is x
		oOdomPoint.y = oTrajectory.pose.pose.position.y;//x in loam is y
		oOdomPoint.z = oTrajectory.pose.pose.position.z;//y in loam is z
		InitializeGridMap(oOdomPoint);
	}
	
	if (!m_bGridMapReadyFlag)
		return;                                                                                

	//record the robot position and update its neighboring map
	if (!(m_iTrajFrameNum % m_iOdomSampingNum)) {

		//ROS_INFO("Nearby region searching.");  

		bSamplingFlag = true;

		//save the odom value
		pcl::PointXYZ oOdomPoint;
		oOdomPoint.x = oTrajectory.pose.pose.position.x;//z in loam is x
		oOdomPoint.y = oTrajectory.pose.pose.position.y;//x in loam is y
		oOdomPoint.z = oTrajectory.pose.pose.position.z;//y in loam is z
		m_vOdomViews.push(oOdomPoint);
		oOdomPoint.z = 0.0;
		m_vOdomShocks.push(oOdomPoint);
		OutputTrajectoryFile(oTrajectory);

		//make the sequence size smaller than a value
		//m_vOdomViews in fact is a history odometry within a given interval
		//it only report the current robot position (back) and past (given time before) position (front) 
		if (m_vOdomViews.size() > m_iPastOdomNum)
			m_vOdomViews.pop();
		if (m_vOdomShocks.size() > m_iShockNum)
			m_vOdomShocks.pop();

		//compute the confidence map on the constructed map with surronding point clouds
		if (m_bGridMapReadyFlag){
			//frequency of visibility calculation should be low
			if(m_iComputedFrame % 3){

				//compute the distance and boundary feature
				ComputeConfidence(m_vOdomViews.back());
		    }else{

		    	//compute all features
		    	ComputeConfidence(m_vOdomViews.back(), m_vOdomViews.front());
			}//end else
		}//end if m_bGridMapReadyFlag
        
        //if the robot is close to the target or the robot is standing in place in a long time
		//if(fRemainDis < 0.5 || fHistoryDis < 0.005){
        if(m_oOPSolver.NearGoal(m_vOdomShocks, 
        	                    m_iShockNum,
        	                    m_iComputedFrame, 2.0, 10)){

			//get the new nodes
			std::vector<int> vNewNodeIdx;
			std::vector<pcl::PointXYZ> vNodeClouds;
			m_oCnfdnSolver.FindLocalMinimum(vNewNodeIdx, vNodeClouds,
	                                        m_vConfidenceMap, m_oGMer, m_iNodeTimes);

			//get new nodes
			m_oOPSolver.GetNewNodeSuppression(m_vConfidenceMap, 
				                                   vNewNodeIdx, 
				                                   vNodeClouds, 1.0);

            //*******use op solver*********
			if(m_oOPSolver.UpdateNodes(m_vConfidenceMap,0.7,0.8))
				//use greedy based method
				m_oOPSolver.GTR(m_vOdomViews.back(),m_vConfidenceMap);
			else
				//use branch and bound based method
				m_oOPSolver.BranchBoundMethod(m_vOdomViews.back(),m_vConfidenceMap);
		    
		    //output
		    std::vector<pcl::PointXYZ> vUnvisitedNodes;
		    m_oOPSolver.OutputUnvisitedNodes(vUnvisitedNodes);

            //clear old data of last trip
		    m_vOdomShocks = std::queue<pcl::PointXYZ>();

		    std::cout<< "remain unvisited nodes are " << vUnvisitedNodes.size()<< std::endl;
            //compute astar path for current target point
            if(vUnvisitedNodes.size()){

            	pcl::PointXYZ oTargetPoint;
            	//get goal
            	m_oOPSolver.OutputGoalPos(oTargetPoint);
            	//update travelable map
		        m_oAstar.UpdateTravelMap(m_oGMer.m_oFeatureMap, m_vConfidenceMap);
                //get raw astar path point clouds
                pcl::PointCloud<pcl::PointXYZ>::Ptr pAstarCloud(new pcl::PointCloud<pcl::PointXYZ>);
   
                //compute astar path
		        bool bPathOptmFlag = m_oAstar.GetPath(pAstarCloud, m_oGMer.m_oFeatureMap,
		                                              m_vOdomViews.back(), oTargetPoint, false);

		        //PublishPointCloud(*pAstarCloud);//for test only

		        std::cout << "optimal or not: " << bPathOptmFlag << std::endl;
		    }
            
            //begin the next trip
            if(m_oOPSolver.CheckNodeTimes())
            	m_iNodeTimes++;
		    
		}//end if m_oOPSolver.NearGoal

        //send generated goal
		if(m_iNodeTimes > 0){
            
			pcl::PointXYZ oTargetPoint;
			//get goal
		    m_oOPSolver.OutputGoalPos(oTargetPoint);
		    //publish goal
            PublishGoalOdom(oTargetPoint);
        }

    }//end if (!(m_iTrajFrameNum % m_iOdomSampingNum))

	//if the frame count touches the least common multiple of m_iOdomSampingNum and 
	//if( bMapUpdateFlag && bSamplingFlag ){
	if (bSamplingFlag) {
		//ROS_INFO("loop touches [%d] and reset at 0.", m_iTrajFrameNum);
		m_iTrajFrameNum = 0;
		m_iComputedFrame++;
	}

	m_iTrajFrameNum++;

}


/*************************************************
Function: HandleTrajectory
Description: a callback function in below:
m_oOdomSuber = node.subscribe(m_sOdomTopic, 5, &GroundExtraction::HandleTrajectory, this);
Calls: none
Called By: TransformLaserInOdom, which is the construction function
Table Accessed: none
Table Updated: none
Input: rawpoint, a 3d point with pcl point type
Output: a point clouds are almost the same with raw point clouds but timestamp values
Return: none
Others: none
*************************************************/

void TopologyMap::HandleGroundClouds(const sensor_msgs::PointCloud2 & vGroundRosData) {

	if (m_bGridMapReadyFlag) {

		////a point clouds in PCL type
		pcl::PointCloud<pcl::PointXYZ> vOneGCloud;
		////message from ROS type to PCL type
		pcl::fromROSMsg(vGroundRosData, vOneGCloud);

        std::vector<int> vNewScanGridIdxs;
		//get right point clouds from LOAM output
		for (int i = 0; i != vOneGCloud.size(); ++i) {
			//sampling 2 time by given sampling value
			if (!(i % (m_iPCSmplNum * 2))) {
				//check the point
				if (m_oGMer.CheckInSidePoint(vOneGCloud.points[i])) {
					//to point clouds
					MapIndex oAllTypeIdx = ExtendedGM::PointoAllTypeIdx(vOneGCloud.points[i],m_oGMer.m_oFeatureMap);

						// If no elevation has been set, use current elevation.
						if (!m_vConfidenceMap[oAllTypeIdx.iOneIdx].label) {
							//to center point cloud
							m_vConfidenceMap[oAllTypeIdx.iOneIdx].oCenterPoint.z = vOneGCloud.points[i].z;
							//to grid layer

							m_vConfidenceMap[oAllTypeIdx.iOneIdx].label = 2;
							vNewScanGridIdxs.push_back(oAllTypeIdx.iOneIdx);
						
						}else{
							//moving average
							float fMeanZ = (m_vConfidenceMap[oAllTypeIdx.iOneIdx].oCenterPoint.z + vOneGCloud.points[i].z) / 2.0f;
							//to center point cloud
							m_vConfidenceMap[oAllTypeIdx.iOneIdx].oCenterPoint.z = fMeanZ;
							
							//cover obstacle grid
							if (m_vConfidenceMap[oAllTypeIdx.iOneIdx].label < 2)
								m_vConfidenceMap[oAllTypeIdx.iOneIdx].label = 2;

						}//end else


				}//end if (CheckInSidePoint(vOneGCloud.points[i]))
			}//end if (!(i%m_iPCSmplNum))
		}//end for (int i = 0; i != vOneGCloud.size();

	}//end if m_bGridMapReadyFlag

}

/*************************************************
Function: HandleTrajectory
Description: a callback function in below:
m_oOdomSuber = node.subscribe(m_sOdomTopic, 5, &GroundExtraction::HandleTrajectory, this);
Calls: none
Called By: TransformLaserInOdom, which is the construction function
Table Accessed: none
Table Updated: none
Input: rawpoint, a 3d point with pcl point type
Output: a point clouds are almost the same with raw point clouds but timestamp values
Return: none
Others: none
*************************************************/

void TopologyMap::HandleBoundClouds(const sensor_msgs::PointCloud2 & vBoundRosData) {
	//if grid map is built
	if (m_bGridMapReadyFlag) {

		////a point clouds in PCL type
		pcl::PointCloud<pcl::PointXYZ> vOneBCloud;
		////message from ROS type to PCL type
		pcl::fromROSMsg(vBoundRosData, vOneBCloud);

		//get boundary points between the ground region and obstacle region
		for (int i = 0; i != vOneBCloud.size(); ++i) {
			//sampling
			if (!(i%m_iPCSmplNum)) {

				if (m_oGMer.CheckInSidePoint(vOneBCloud.points[i])) {
					m_pBoundCloud->points.push_back(vOneBCloud.points[i]);
					int iPointIdx = ExtendedGM::PointoOneDIdx(vOneBCloud.points[i],m_oGMer.m_oFeatureMap);
					//to point idx
					m_vBoundPntMapIdx[iPointIdx].push_back(m_iBoundFrames);
					//if this grid has not been found as a boundary region
					if (m_vConfidenceMap[iPointIdx].label != 3) {
						//label as boundary grid
					    m_vConfidenceMap[iPointIdx].label = 3;
					    //search its neighboring region (region grow scale)
					    std::vector<int> vNearGridIdx;
				        ExtendedGM::CircleNeighborhood(vNearGridIdx,
						                               m_oGMer.m_oFeatureMap, 
											           m_oGMer.m_vBoundDefendMask,
		                                               iPointIdx);

				        //label as non-travelable region since it is dangerous for robot to close to obstacle in a distance
                        for(int i = 0; i != vNearGridIdx.size(); ++i)
                        	m_vConfidenceMap[vNearGridIdx[i]].travelable = 4;

				    }//end if

					m_iBoundFrames++;
				}
			}//end if i%m_iPCSmplNum

		}//end for


		if(m_pBoundCloud->points.size()>3000000){
			SamplingPointClouds(m_pBoundCloud, m_vBoundPntMapIdx);
		}//end if if(m_pBoundCloud->points.size()>X)

	}//if m_bGridMapReadyFlag

}

/*************************************************
Function: HandleTrajectory
Description: a callback function in below:
m_oOdomSuber = node.subscribe(m_sOdomTopic, 5, &GroundExtraction::HandleTrajectory, this);
Calls: none
Called By: TransformLaserInOdom, which is the construction function
Table Accessed: none
Table Updated: none
Input: rawpoint, a 3d point with pcl point type
Output: a point clouds are almost the same with raw point clouds but timestamp values
Return: none
Others: none
*************************************************/

void TopologyMap::HandleObstacleClouds(const sensor_msgs::PointCloud2 & vObstacleRosData) {

	if (m_bGridMapReadyFlag) {

		////a point clouds in PCL type
		pcl::PointCloud<pcl::PointXYZ> vOneOCloud;
		////message from ROS type to PCL type
		pcl::fromROSMsg(vObstacleRosData, vOneOCloud);

		//get obstacle points
		for (int i = 0; i != vOneOCloud.size(); ++i) {
			//sampling
			if (!(i%m_iPCSmplNum)) {
                //if this point is inside the map
				if (m_oGMer.CheckInSidePoint(vOneOCloud.points[i])) {
                    //if the obstacle cloud 
                    //save the obstacle point
					m_pObstacleCloud->points.push_back(vOneOCloud.points[i]);
					//save the corresponding node times
					m_vObstNodeTimes.push_back(m_iNodeTimes);
                    //send point index to grid member
					int iPointIdx = ExtendedGM::PointoOneDIdx(vOneOCloud.points[i],m_oGMer.m_oFeatureMap);
					//to point idx
					m_vObstlPntMapIdx[iPointIdx].push_back(m_iObstacleFrames);

					//the obstacle grid can cover unknown, ground, obstacle grids in simulation
					if(!m_vConfidenceMap[iPointIdx].label) {
						//label grid as obstacle grid
						m_vConfidenceMap[iPointIdx].label = 1;
						//
						m_vConfidenceMap[iPointIdx].travelable = 4;
						
					}
					/*add some conditions here if ground points has some noise (to cover ground grid)*/

					m_iObstacleFrames++;
				}//end if m_oGMer.CheckInSidePoint(vOneOCloud.points[i])
			}//end if i%m_iPCSmplNum
		}//end for i


		if(m_pObstacleCloud->points.size()>8000000){
			SamplingPointClouds(m_pObstacleCloud, m_vObstlPntMapIdx, m_vObstNodeTimes);
		}//end if if(m_pObstacleCloud->points.size()>X)


	}//end if (m_bGridMapReadyFlag) 

}



//*************Feature calculation function (Subject function)*************

/*************************************************
Function: TopologyMap
Description: constrcution function for TopologyMap class
Calls: all member functions
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: global node,
privare node
flag of generating output file
original frame value
Output: none
Return: none
Others: the HandlePointClouds is the kernel function
*************************************************/

void TopologyMap::ComputeConfidence(const pcl::PointXYZ & oCurrRobotPos,
	                                const pcl::PointXYZ & oPastRobotPos) {

	pcl::PointCloud<pcl::PointXYZ>::Ptr pNearGrndClouds(new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr pNearBndryClouds(new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr pNearAllClouds(new pcl::PointCloud<pcl::PointXYZ>);
    std::vector<int> vNearGrndGrdIdxs;

    //find the neighboring point clouds
    std::vector<MapIndex> vNearByIdxs;
	ExtendedGM::CircleNeighborhood(vNearByIdxs, 
		                           m_oGMer.m_oFeatureMap, 
		                           m_oGMer.m_vRobotSearchMask,
		                           oCurrRobotPos);

    //label the node count of computed ground grids 
	//grow the travelable region
	m_oCnfdnSolver.RegionGrow(m_vConfidenceMap,
			                  vNearByIdxs,
			                  m_oGMer,
			                  m_iNodeTimes);

    //extract point clouds with different labels, respectively
    DevidePointClouds(*pNearGrndClouds,
    	              *pNearBndryClouds,
    	              *pNearAllClouds,
	                  vNearGrndGrdIdxs,
                      vNearByIdxs);


    //compute distance term
    m_oCnfdnSolver.DistanceTerm(m_vConfidenceMap,
    	                           oCurrRobotPos,
                                vNearGrndGrdIdxs,
	                            *pNearGrndClouds);


    //in this case, robot position is based on odom frame, it need to be transfored to lidar sensor frame 
    pcl::PointXYZ oPastView;
    oPastView.x = oPastRobotPos.x;
    oPastView.y = oPastRobotPos.y;
    oPastView.z = oPastRobotPos.z + m_fViewZOffset;

    //compute visibiity
    if(vNearGrndGrdIdxs.size() >= 3)
    	m_oCnfdnSolver.OcclusionTerm(m_vConfidenceMap,
	                                   pNearAllClouds,
	                                 vNearGrndGrdIdxs,
	                                        oPastView,
	                                     m_iNodeTimes);


    //compute boundary term
    m_oCnfdnSolver.BoundTerm(m_vConfidenceMap,
                             vNearGrndGrdIdxs,
	                         pNearGrndClouds,
    	                     pNearBndryClouds);


    //publish result
	//PublishPointCloud(*pNearGrndClouds);//for test
	//PublishPointCloud(*pNearBndryClouds);//for test
	PublishPointCloud(*pNearAllClouds);//for test
	PublishGridMap();

}
//reload without occlusion
void TopologyMap::ComputeConfidence(const pcl::PointXYZ & oCurrRobotPos) {

	pcl::PointCloud<pcl::PointXYZ>::Ptr pNearGrndClouds(new pcl::PointCloud<pcl::PointXYZ>);
	pcl::PointCloud<pcl::PointXYZ>::Ptr pNearBndryClouds(new pcl::PointCloud<pcl::PointXYZ>);
	
    std::vector<int> vNearGrndGrdIdxs;

    //find the neighboring point clouds
    std::vector<MapIndex> vNearByIdxs;
	ExtendedGM::CircleNeighborhood(vNearByIdxs,
	                               m_oGMer.m_oFeatureMap,
	                               m_oGMer.m_vRobotSearchMask,
	                               oCurrRobotPos);
	
    //label the node count of computed ground grids 
	//grow the travelable region
	m_oCnfdnSolver.RegionGrow(m_vConfidenceMap,
			                  vNearByIdxs,
			                  m_oGMer,
			                  m_iNodeTimes);

    //extract point clouds with different labels, respectively
    DevidePointClouds(*pNearGrndClouds,
                     *pNearBndryClouds,
                      vNearGrndGrdIdxs,
                           vNearByIdxs);

    //compute distance term
    m_oCnfdnSolver.DistanceTerm(m_vConfidenceMap,
    	                           oCurrRobotPos,
                                vNearGrndGrdIdxs,
	                            *pNearGrndClouds);


    //compute boundary term
    m_oCnfdnSolver.BoundTerm(m_vConfidenceMap,
                             vNearGrndGrdIdxs,
	                          pNearGrndClouds,
    	                     pNearBndryClouds);


    //compute quality term
    m_oCnfdnSolver.QualityTerm(m_vConfidenceMap,
    	                       m_pObstacleCloud,
                               m_vObstNodeTimes,
                              m_vObstlPntMapIdx, 
		                                m_oGMer,
		                            vNearByIdxs,
                                   m_iNodeTimes, 5);



    //publish result
	//PublishPointCloud(*pNearGrndClouds);//for test
	//PublishPointCloud(*pNearBndryClouds);//for test
	
	//output result on screen
	PublishGridMap();

}
//*********************************Traversing / retrieving function*********************************
/*************************************************
Function: TopologyMap
Description: constrcution function for TopologyMap class
Calls: all member functions
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: global node,
       privare node
       flag of generating output file
       original frame value
Output: none
Return: none
Others: the HandlePointClouds is the kernel function
*************************************************/

void TopologyMap::PublishGridMap(){

	int iTravelableNum = 0;

    //push 
	grid_map::Matrix& gridMapData1 = m_oGMer.m_oFeatureMap["elevation"];
	grid_map::Matrix& gridMapData2 = m_oGMer.m_oFeatureMap["traversability"];
	grid_map::Matrix& gridMapData3 = m_oGMer.m_oFeatureMap["boundary"];
	grid_map::Matrix& gridMapData4 = m_oGMer.m_oFeatureMap["observability"];
	grid_map::Matrix& gridMapData5 = m_oGMer.m_oFeatureMap["confidence"];
	grid_map::Matrix& gridMapData6 = m_oGMer.m_oFeatureMap["travelable"];
	grid_map::Matrix& gridMapData7 = m_oGMer.m_oFeatureMap["quality"];

	//initial elevation map and center point clouds
	for (int i = 0; i != m_oGMer.m_oFeatureMap.getSize()(0); ++i) {//i

		for (int j = 0; j != m_oGMer.m_oFeatureMap.getSize()(1); ++j) {//j

			int iGridIdx = ExtendedGM::TwotoOneDIdx(i, j);

		    //render travelable ground grid    
			//if(m_vConfidenceMap[iGridIdx].label == 2){
 
                //record the region that has been explored
                if(m_vConfidenceMap[iGridIdx].travelable == 1)
                	iTravelableNum++;

                //assign computed resultes
		    	gridMapData1(i, j) = m_vConfidenceMap[iGridIdx].nodeCount;//.qualTerm
		    	gridMapData2(i, j) = m_vConfidenceMap[iGridIdx].travelTerm;//.travelTerm
		    	gridMapData3(i, j) = m_vConfidenceMap[iGridIdx].boundTerm;//.boundTerm
		    	gridMapData4(i, j) = m_vConfidenceMap[iGridIdx].visiTerm.value;//.visiTerm
		    	gridMapData5(i, j) = m_vConfidenceMap[iGridIdx].totalValue;//.totalValue
		    	gridMapData6(i, j) = m_vConfidenceMap[iGridIdx].travelable;//.travelable
		    	//gridMapData6(i, j) = m_oAstar.maze[i][j];//test only
		    	//gridMapData7(i, j) = m_vConfidenceMap[iGridIdx].qualTerm;//quality term
            
		   // }else{
		    	
                //assign limited resultes
		  //  	gridMapData1(i, j) = std::numeric_limits<float>::infinity();
		  //  	gridMapData2(i, j) = std::numeric_limits<float>::infinity();
		   // 	gridMapData3(i, j) = std::numeric_limits<float>::infinity();
		   // 	gridMapData4(i, j) = std::numeric_limits<float>::infinity();
		   // 	gridMapData5(i, j) = std::numeric_limits<float>::infinity();
		   // 	gridMapData6(i, j) = std::numeric_limits<float>::infinity();
		    	//gridMapData7(i, j) = std::numeric_limits<float>::infinity();

		   // }//end else

            //quality is in boundary and obstacle grid
		    gridMapData7(i, j) = m_vConfidenceMap[iGridIdx].qualTerm;//quality term

		}//end j

	}//end i

    //output cover rate file
	OutputCoverRateFile(iTravelableNum);

	ros::Time oNowTime = ros::Time::now();

	m_oGMer.m_oFeatureMap.setTimestamp(oNowTime.toNSec());

	grid_map_msgs::GridMap oGridMapMessage;
	grid_map::GridMapRosConverter::toMessage(m_oGMer.m_oFeatureMap, oGridMapMessage);
	// Publish as grid map.
	m_oGridMapPublisher.publish(oGridMapMessage);

}

/*************************************************
Function: TopologyMap
Description: constrcution function for TopologyMap class
Calls: all member functions
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: global node,
       privare node
       flag of generating output file
       original frame value
Output: none
Return: none
Others: the HandlePointClouds is the kernel function
*************************************************/

void TopologyMap::PublishPointCloud(pcl::PointCloud<pcl::PointXYZ> & vCloud){
  //publish obstacle points
  sensor_msgs::PointCloud2 vCloudData;

  pcl::toROSMsg(vCloud, vCloudData);

  vCloudData.header.frame_id = m_oGMer.m_oFeatureMap.getFrameId();

  vCloudData.header.stamp = ros::Time::now();

  m_oCloudPublisher.publish(vCloudData);

}


/*************************************************
Function: TopologyMap
Description: constrcution function for TopologyMap class
Calls: all member functions
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: global node,
       privare node
       flag of generating output file
       original frame value
Output: none
Return: none
Others: the HandlePointClouds is the kernel function
*************************************************/

void TopologyMap::PublishGoalOdom(pcl::PointXYZ & oGoalPoint){

        nav_msgs::Odometry oCurrGoalOdom;
        oCurrGoalOdom.header.stamp = ros::Time::now();
        oCurrGoalOdom.header.frame_id = "odom";

        //set the position
        oCurrGoalOdom.pose.pose.position.x = oGoalPoint.x;
        oCurrGoalOdom.pose.pose.position.y = oGoalPoint.y;
        oCurrGoalOdom.pose.pose.position.z = oGoalPoint.z;

        m_oGoalPublisher.publish(oCurrGoalOdom);

}


/*************************************************
Function: TopologyMap
Description: constrcution function for TopologyMap class
Calls: all member functions
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: global node,
       privare node
       flag of generating output file
       original frame value
Output: none
Return: none
Others: the HandlePointClouds is the kernel function
*************************************************/

void TopologyMap::OutputCoverRateFile(const int & iTravelableNum){


	if(!m_bCoverFileFlag){

	    //set the current time stamp as a file name
        //full name 
		m_sCoverFileName << m_sFileHead << "CoverRes_" << ros::Time::now() << ".txt"; 

		m_bCoverFileFlag = true;
        //print coverage rate evaluation message
		std::cout << "Attention a coverage rate evaluation file is created in " << m_sCoverFileName.str() << std::endl;
	}

	//output
	m_oCoverFile.open(m_sCoverFileName.str(), std::ios::out | std::ios::app);

	//output in a txt file
    //the storage type of output file is x y z time frames right/left_sensor
    m_oCoverFile << iTravelableNum << " "
                 << ros::Time::now() << " "
                 << std::endl;

    m_oCoverFile.close();

}

/*************************************************
Function: TopologyMap
Description: constrcution function for TopologyMap class
Calls: all member functions
Called By: main function of project
Table Accessed: none
Table Updated: none
Input: global node,
       privare node
       flag of generating output file
       original frame value
Output: none
Return: none
Others: the HandFindLocalMinimumlePointClouds is the kernel function
*************************************************/

void TopologyMap::OutputTrajectoryFile(const nav_msgs::Odometry & oTrajectory){


	if(!m_bOutTrajFileFlag){

	    //set the current time stamp as a file name
        //full name 
		m_sOutTrajFileName << m_sFileHead << "Traj_" << oTrajectory.header.stamp << ".txt"; 

		m_bOutTrajFileFlag = true;
        //print output file generation message
		std::cout << "[*] Attention a trajectory recording file is created in " << m_sOutTrajFileName.str() << std::endl;
	}

	//output
	m_oTrajFile.open(m_sOutTrajFileName.str(), std::ios::out | std::ios::app);

	//output in a txt file
	//the storage type of output file is x y z time frames 
    m_oTrajFile << oTrajectory.pose.pose.position.x << " "
                << oTrajectory.pose.pose.position.y << " "
                << oTrajectory.pose.pose.position.z << " " 
                << oTrajectory.header.stamp << " "
                << std::endl;

    m_oTrajFile.close();

}





} /* namespace */





