// Udacity C3 Localization
// Dec 21 2020
// Aaron Brown

using namespace std;

#include <string>
#include <sstream>
#include "helper.h"
#include<iostream>

#include <Eigen/Core>
#include <Eigen/SVD>
using namespace Eigen;
#include <pcl/registration/icp.h>
#include <pcl/console/time.h>   // TicToc

Pose pose(Point(0,0,0), Rotate(0,0,0));
Pose upose = pose;
vector<int> associations;
vector<int> bestAssociations = {5,6,7,8,9,10,11,12,13,14,15,16,16,17,18,19,20,21,22,23,24,25,26,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,62,63,64,65,66,67,68,69,70,71,72,74,75,76,77,78,79,80,81,82,83,84,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,122,123,124,125,126,127,0,1,2,3,4,4};
bool init = false;
bool matching = false;
bool update = false;
void keyboardEventOccurred(const pcl::visualization::KeyboardEvent &event, void* viewer)
{

  	//boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer = *static_cast<boost::shared_ptr<pcl::visualization::PCLVisualizer> *>(viewer_void);
	if (event.getKeySym() == "Right" && event.keyDown()){
		update = true;
		upose.position.x += 0.1;
  	}
	else if (event.getKeySym() == "Left" && event.keyDown()){
		update = true;
		upose.position.x -= 0.1;
  	}
  	if (event.getKeySym() == "Up" && event.keyDown()){
		update = true;
		upose.position.y += 0.1;
  	}
	else if (event.getKeySym() == "Down" && event.keyDown()){
		update = true;
		upose.position.y -= 0.1;
  	}
	else if (event.getKeySym() == "k" && event.keyDown()){
		update = true;
		upose.rotation.yaw += 0.1;
		while( upose.rotation.yaw > 2*pi)
			upose.rotation.yaw -= 2*pi;  
  	}
	else if (event.getKeySym() == "l" && event.keyDown()){
		update = true;
		upose.rotation.yaw -= 0.1;
		while( upose.rotation.yaw < 0)
			upose.rotation.yaw += 2*pi; 
  	}
	else if (event.getKeySym() == "space" && event.keyDown()){
		matching = true;
		update = false;
  	}
	else if (event.getKeySym() == "n" && event.keyDown()){
		pose = upose;
		cout << "Set New Pose" << endl;
  	}
	else if (event.getKeySym() == "b" && event.keyDown()){
		cout << "Do ICP With Best Associations" << endl;
		matching = true;
		update = true;
  	}
}

double Score(vector<int> pairs, PointCloudT::Ptr target, PointCloudT::Ptr source, Eigen::Matrix4d transform){
	//Yoshida: this function calculate the matching score after moving source PCT by transform matrix
	double score = 0;
	int index = 0;
	for(int i : pairs){
		Eigen::MatrixXd p(4, 1);
		p(0,0) = (*source)[index].x;
		p(1,0) = (*source)[index].y;
		p(2,0) = 0.0;
		p(3,0) = 1.0;
		Eigen::MatrixXd p2 = transform * p;
		PointT association = (*target)[i];
		score += sqrt( (p2(0,0) - association.x) * (p2(0,0) - association.x) + (p2(1,0) - association.y) * (p2(1,0) - association.y) );
		index++;
	}
	return score;
}

vector<int> NN(PointCloudT::Ptr target, PointCloudT::Ptr source, Eigen::Matrix4d initTransform, double dist){
	
	vector<int> associations;

	// TODO: complete this function which returns a vector of target indicies that correspond to each source index inorder.
	// E.G. source index 0 -> target index 32, source index 1 -> target index 5, source index 2 -> target index 17, ... 
	
	// TODO: create a KDtree with target as input
	pcl::KdTreeFLANN<PointT> kdtree;
	kdtree.setInputCloud(target);

	// TODO: transform source by initTransform
	PointCloudT::Ptr transformSource (new PointCloudT);
	pcl::transformPointCloud(*source,*transformSource, initTransform);


	// TODO loop through each transformed source point and using the KDtree find the transformed source point's nearest target point. Append the nearest point to associaitons 

	//Yoshida: need to create loop per point here?

	for (PointT point : transformSource -> points){
		vector<int> pointIdxRadiusSearch;
		vector<float> pointRadiusSquaredDistance;

		//pcl::PointXYZ SourcePoint = transformSource ->points[i];

		if(kdtree.nearestKSearch(point, dist, pointIdxRadiusSearch, pointRadiusSquaredDistance) > 0){
			associations.push_back(pointIdxRadiusSearch[0]);
			}

		else{
			associations.push_back(-1);
		}
	}

	return associations;
}

vector<Pair> PairPoints(vector<int> associations, PointCloudT::Ptr target, PointCloudT::Ptr source, bool render, pcl::visualization::PCLVisualizer::Ptr& viewer){

	vector<Pair> pairs;

	// TODO: loop through each source point and using the corresponding associations append a Pair of (source point, associated target point)
	for(int i =0; i < (*source).size(); i++){

		PointT point = (*source).points[i];

		if(associations[i]>=0){

			PointT association = (*target).points[associations[i]];

			viewer->removeShape(to_string(i));
			renderRay(viewer, Point(point.x, point.y,0), Point(association.x,association.y,0), to_string(i), Color(0,1,0));
		
		//Pair pairs(Point(point.x,point.y,0),Point(association.x,association.y,0)); Not Good

			pairs.push_back(Pair(Point(point.x,point.y,0),Point(association.x,association.y,0)));

		

		}
	}
	return pairs;
}

Eigen::Matrix4d ICP(vector<int> associations, PointCloudT::Ptr target, PointCloudT::Ptr source, Pose startingPose, int iterations, pcl::visualization::PCLVisualizer::Ptr& viewer){

  	//Eigen::Matrix4d transformation_matrix = Eigen::Matrix4d::Identity();

  	// TODO: transform source by startingPose
	cout <<"check1" <<endl;

	Eigen::Matrix4d startingPoseMat = transform3D(startingPose.rotation.yaw, startingPose.rotation.pitch, startingPose.rotation.roll, startingPose.position.x, startingPose.position.y, startingPose.position.z);

	cout <<"check2" <<endl;

	PointCloudT::Ptr sourceTransformed(new PointCloudT);
	pcl::transformPointCloud(*source, *sourceTransformed, startingPoseMat);

  	// TODO: create matrices P and Q which are both 2 x 1 and represent mean point of pairs 1 and pairs 2 respectivley.
  	// In other words P is the mean point of source and Q is the mean point target 
  	// P = [ mean p1 x] Q = [ mean p2 x]
  	//	   [ mean p1 y]	    [ mean p2 y]

	vector<Pair> pairs = PairPoints(associations, target, sourceTransformed, true, viewer);


	//double total_px, total_py, total_qx, total_qy;
	int totalPointNum = (*source).points.size();

	

	Eigen::MatrixXd P(2,1);
	Eigen::MatrixXd Q(2,1);

	for(Pair pair : pairs){
		P(0,0) += pair.p1.x;
		P(1,0) += pair.p1.y;
		Q(0,0) += pair.p2.x;
		Q(1,0) += pair.p2.y;
	}

	P(0,0) = P(0,0)/pairs.size();
	P(1,0) = P(1,0)/pairs.size();
	Q(0,0) = Q(0,0)/pairs.size();
	Q(1,0) = Q(1,0)/pairs.size();

	cout <<"check3" <<endl;

  	// TODO: get pairs of points from PairPoints and create matrices X and Y which are both 2 x n where n is number of pairs.
  	// X is pair 1 x point with pair 2 x point for each column and Y is the same except for y points
  	// X = [p1 x0 , p1 x1 , p1 x2 , .... , p1 xn ] - [Px]   Y = [p2 x0 , p2 x1 , p2 x2 , .... , p2 xn ] - [Qx]
  	//     [p1 y0 , p1 y1 , p1 y2 , .... , p1 yn ]   [Py]       [p2 y0 , p2 y1 , p2 y2 , .... , p2 yn ]   [Qy]

	Eigen::MatrixXd X(2,pairs.size());
	Eigen::MatrixXd Y(2,pairs.size());

	int i=0;

	for(Pair pair : pairs){
		X(0,i) = pair.p1.x - P(0,0);
		X(1,i) = pair.p1.y - P(1,0);
		Y(0,i) = pair.p2.x - Q(0,0);
		Y(1,i) = pair.p2.y - Q(1,0);
		i++;
	}

  	// TODO: create matrix S using equation 3 from the svd_rot.pdf. Note W is simply the identity matrix because weights are all 1

	Eigen::MatrixXd W = Eigen::MatrixXd::Identity (totalPointNum,totalPointNum);
	Eigen::MatrixXd S = X*W*(Y.transpose());
  	// TODO: create matrix R, the optimal rotation using equation 4 from the svd_rot.pdf and using SVD of S

	Eigen::JacobiSVD<Eigen::MatrixXd> svd(S, Eigen::ComputeFullV | Eigen::ComputeFullU);

	Eigen::MatrixXd U = svd.matrixU();
	Eigen::VectorXd sigma = svd.singularValues();
	Eigen::MatrixXd V = svd.matrixV();

	Eigen::MatrixXd V_UT = V * U.transpose();
	Eigen::MatrixXd diag = Eigen::MatrixXd::Identity(V.cols(),U.cols());
	diag(V.cols()-1,U.cols()-1) = V_UT.determinant();

	cout <<"check4" <<endl;

	cout<<V.cols()<<V.rows()<<diag.cols()<<diag.rows()<<U.transpose().cols()<<U.transpose().rows()<<endl;

	Eigen::MatrixXd R = V*diag*(U.transpose());


	cout <<"check5" <<endl;
  	// TODO: create mtarix t, the optimal translatation using equation 5 from svd_rot.pdf
	Eigen::MatrixXd t = Q - R*P;


  	// TODO: set transformation_matrix based on above R, and t matrices
  	// [ R R 0 t]
  	// [ R R 0 t]
  	// [ 0 0 1 0]
  	// [ 0 0 0 1]

	Eigen::Matrix4d transformation_matrix = Eigen::Matrix4d::Identity(4,4);
	transformation_matrix(0,0) = R(0,0);
	transformation_matrix(0,1) = R(0,1);
	transformation_matrix(1,0) = R(1,0);
	transformation_matrix(1,1) = R(1,1);
	transformation_matrix(0,3) = t(0,0);
	transformation_matrix(1,3) = t(1,0);
	
	cout <<"check6" <<endl;
	cout <<transformation_matrix(0,0)<<transformation_matrix(0,1)<<transformation_matrix(1,0)<<transformation_matrix(1,1)<<transformation_matrix(0,3)<<transformation_matrix(1,3) <<endl;
	
	///this may be very important!!!!
	transformation_matrix = transformation_matrix * startingPoseMat;


  	return transformation_matrix;
}

int main(){

	pcl::visualization::PCLVisualizer::Ptr viewer (new pcl::visualization::PCLVisualizer ("ICP Creation"));
  	viewer->setBackgroundColor (0, 0, 0);
  	viewer->addCoordinateSystem (1.0);
	viewer->registerKeyboardCallback(keyboardEventOccurred, (void*)&viewer);

	// Load target
	PointCloudT::Ptr target(new PointCloudT);
  	pcl::io::loadPCDFile("target.pcd", *target);

	// Load source
	PointCloudT::Ptr source(new PointCloudT);
  	pcl::io::loadPCDFile("source.pcd", *source);

	renderPointCloud(viewer, target, "target", Color(0,0,1));
	renderPointCloud(viewer, source, "source", Color(1,0,0));
	viewer->addText("Score", 200, 200, 32, 1.0, 1.0, 1.0, "score",0);

  	while (!viewer->wasStopped ())
  	{
		if(matching){

			init = true;
			
			viewer->removePointCloud("usource");

			Eigen::Matrix4d transformInit = transform3D(pose.rotation.yaw, pose.rotation.pitch, pose.rotation.roll, pose.position.x, pose.position.y, pose.position.z);
			PointCloudT::Ptr transformed_scan (new PointCloudT);
  			pcl::transformPointCloud (*source, *transformed_scan, transformInit);
			viewer->removePointCloud("source");
  			renderPointCloud(viewer, transformed_scan, "source", Color(1,0,0));

			if(!update){
				associations = NN(target, source, transformInit, 5);
				cout << "association updated!" <<endl;
			}
			else{
				associations = bestAssociations;
				cout << "keep best association" <<endl;
			}
		

			Eigen::Matrix4d transform = ICP(associations, target, source, pose, 1, viewer);

			pose = getPose(transform);
  			pcl::transformPointCloud (*source, *transformed_scan, transform);
			viewer->removePointCloud("icp_scan");
  			renderPointCloud(viewer, transformed_scan, "icp_scan", Color(0,1,0));

			double score = Score(associations,  target, source, transformInit);
			viewer->removeShape("score");
			viewer->addText("Score: "+to_string(score), 200, 200, 32, 1.0, 1.0, 1.0, "score",0);
			double icpScore = Score(associations,  target, source, transform);
			viewer->removeShape("iscore");
			viewer->addText("ICP Score: "+to_string(icpScore), 200, 150, 32, 1.0, 1.0, 1.0, "iscore",0);

			matching = false;
			update = false;
			upose = pose;
			
		}
		else if(update && init){

			Eigen::Matrix4d userTransform = transform3D(upose.rotation.yaw, upose.rotation.pitch, upose.rotation.roll, upose.position.x, upose.position.y, upose.position.z);

			PointCloudT::Ptr transformed_scan (new PointCloudT);
  			pcl::transformPointCloud (*source, *transformed_scan, userTransform);
			viewer->removePointCloud("usource");
			renderPointCloud(viewer, transformed_scan, "usource", Color(0,1,1));

			vector<Pair> pairs = PairPoints(associations, target, transformed_scan, true, viewer);

			double score = Score(associations,  target, source, userTransform);
			viewer->removeShape("score");
			viewer->addText("Score: "+to_string(score), 200, 200, 32, 1.0, 1.0, 1.0, "score",0);
			
			update = false;
			
		}

  		viewer->spinOnce ();
  	}
  	
	return 0;
}
