#ifndef FINDCAMERAMATRICES_H
#define FINDCAMERAMATRICES_H


#include <vector>
#include <utility>

#include <opencv2/opencv.hpp>


using namespace std;

bool TestTriangulation(const vector<cv::Point3f>& pcloud_pt3d, const cv::Matx34f& P);
void getFundamentalMatrix(pair<vector<cv::Point2f>, vector<cv::Point2f>> const& points, vector<cv::Point2f> *inliers1, vector<cv::Point2f> *inliers2, cv::Mat& F);
bool CheckCoherentRotation(const cv::Mat& R);
bool DecomposeEtoRandT(cv::Mat E, cv::Mat_<double>& R1, cv::Mat_<double>& R2, cv::Mat_<double>& t1, cv::Mat_<double>& t2);
bool getRightProjectionMat(cv::Mat& E, const cv::Mat K, const cv::Mat KInv, const cv::Mat distCoeff, const vector<cv::Point2f>& inliersF1, const vector<cv::Point2f>& inliersF2, cv::Matx34f& P1, std::vector<cv::Point3f>& outCloud);


#endif // FINDCAMERAMATRICES_H
