#include "line/MyLine.h"
#include "FindCameraMatrices.h"
#include "FindPoints.h"
#include "MultiCameraPnP.h"
#include "Triangulation.h"
#include "Visualisation.h"
#include "PointCloudVis.h"

#include <cmath>
#include <math.h>
#include <vector>
#include <utility>
#include <stack>
#include <sstream>
#include <string.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
//#include "opencv2/nonfree/nonfree.hpp"

using namespace std;

char key;

//TODO:
// other meothod do decompose essential mat;
//      http://www.morethantechnical.com/2012/08/09/decomposing-the-essential-matrix-using-horn-and-eigen-wcode/
//
/* STEP BY STEP:
 * 1.  capture stereo calibrated images in frame 1
 * 2.1 find feature points in image 1.1
 * 2.2 find corresponding points in image 1.2
 * 3.  triangulate 3d points from frame 1
 * 4.  wait one frame
 * 5.  capture again images from frame 2
 * 6.  try to find same corresponding points from frame 1 in new stereo images from frame 2
 * 7.  triangulate 3d points from frame 2
 * 8.  calculate essential matrix from frame 1 to frame 2
 * 9.  estimate motion with 2 sets of 2d and 3d points and the essential matrix .. how?
 * 10. swap 2d points of frame 1 and frame 2
 * 11. try to add some new feature points (until defined numer of necessary points are reached)
 * 12. continue with step 4
 *
 * surf detection need nonfree lib... can't use it in the vr-lab
 */

//callback
void method2 (const std::vector<cv::Point3f> &worldcoordinates, const std::vector<cv::Point2f>& point_L2, const std::vector<cv::Point2f>& point_R2, const cv::Mat &P_L, const cv::Mat &P_R, const cv::Mat &K_L, const cv::Mat &K_R, cv::Mat& T_L, cv::Mat& T_R, cv::Mat& R_L, cv::Mat& R_R);
void method3 (const std::vector<cv::Point2f> &point_L1, const std::vector<cv::Point2f>& point_R1, const std::vector<cv::Point2f>& point_L2, const std::vector<cv::Point2f>& point_R2, const cv::Mat& P_LR, cv::Mat& T);

int main() {

    int frame=1;
    // get calibration Matrix K
    cv::Mat K_L, distCoeff_L, K_R, distCoeff_R;
    loadIntrinsic("left", K_L, distCoeff_L);
    loadIntrinsic("right", K_R, distCoeff_R);

    // get extrinsic test parameter
    cv::Mat E_LR, F_LR, R_LR, T_LR;
    loadExtrinsic(R_LR, T_LR, E_LR, F_LR);

    // calculate inverse K
    cv::Mat KInv_L, KInv_R;
    cv::invert(K_L, KInv_L);
    cv::invert(K_L, KInv_R);

    // get projection Mat between L and R
    cv::Mat P_LR;
    cv::hconcat(R_LR, T_LR, P_LR);

    cv::Mat P0 = (cv::Mat_<double>(3,4) <<
                  1.0, 0.0, 0.0, 0.0,
                  0.0, 1.0, 0.0, 0.0,
                  0.0, 0.0, 1.0, 0.0 );


    // define image size
    int resX = 752;
    int resY = 480;

    // currentPosition E Mat
    cv::Mat positionL = cv::Mat::eye(4, 4, CV_64F);
    cv::Mat positionR = cv::Mat::eye(4, 4, CV_64F);

    // currentPosition SOLVE PNP RANSAC
    cv::Mat position2L = cv::Mat::eye(4, 4, CV_64F);
    cv::Mat position2R = cv::Mat::eye(4, 4, CV_64F);

    // currentPosition TRIANGULATION
    cv::Mat position3L = cv::Mat::eye(4, 4, CV_64F);
    cv::Mat position3R = cv::Mat::eye(4, 4, CV_64F);

//    cv::Point2f currentPos_L1(900, 200);
//    cv::Point2f currentPos_R1(900, 200);
//    cv::Point2f currentPos_L2(900, 200);
//    cv::Point2f currentPos_R2(900, 200);
//    cv::Point2f currentPos_L3(500, 500);
//    cv::Point2f currentPos_R3(500, 500);
//    cv::Point2f currentPos_4(500, 500);

//    cv::Mat path1 = cv::imread("data/background.jpg");
//    cv::Mat path2 = cv::imread("data/background.jpg");
//    cv::Mat path3 = cv::imread("data/background.jpg");
//    cv::Mat path4 = cv::imread("data/background.jpg");

//    cv::namedWindow("motionPath 1", cv::WINDOW_NORMAL);
//    cv::namedWindow("motionPath 2", cv::WINDOW_NORMAL);
//    cv::namedWindow("motionPath 3", cv::WINDOW_NORMAL);
//    cv::namedWindow("motionPath 4", cv::WINDOW_NORMAL);

    while(true)
    {
        cout << "FRAME" <<  frame << endl;
        // ************************************
        // ******* Motion Estimation **********
        // ************************************
        // 1- Get Matrix K
        // 2. calculate EssentialMatrix
        // 3. for bundle adjustment use SSBA
        // 4. or http://stackoverflow.com/questions/13921720/bundle-adjustment-functions
        // 5. recover Pose (need newer version of calib3d)

        //stereo1
        cv::Mat frame_L1 = cv::imread("data/stereoImages/left/"+(std::to_string(frame))+"_l.jpg",0);
        cv::Mat frame_R1 = cv::imread("data/stereoImages/right/"+(std::to_string(frame))+"_r.jpg",0);

        //stereo2
        cv::Mat frame_L2 = cv::imread("data/stereoImages/left/"+(std::to_string(frame+1))+"_l.jpg",0);
        cv::Mat frame_R2 = cv::imread("data/stereoImages/right/"+(std::to_string(frame+1))+"_r.jpg",0);

        // Check for invalid input
        if(! frame_L1.data || !frame_R1.data || !frame_R2.data || !frame_L2.data) {
            cout <<  "Could not open or find the image: "  << std::endl ;
            //frame=1;
            continue;
        }

        std::vector<cv::Point2f> points_L1, points_R1, points_L2, points_R2;
        findCorresPoints_LucasKanade(frame_L1, frame_R1, frame_L2, frame_R2, &points_L1, &points_R1, &points_L2, &points_R2);



        // Test TRIANGULATION
//        {
//            deleteZeroLines(points_L1, points_R1);

//            if (0 == points_L1.size()) {
//                cout << "to less points found" << endl;
//                ++frame;
//                continue;
//            }

//            std::vector<cv::Point2f> normP_L1, normP_R1;
//            normalizePoints(KInv_L, KInv_R, points_L1, points_R1, normP_L1, normP_R1);

//            cv::Mat color_image;
//            cv::cvtColor(frame_L1, color_image, CV_GRAY2RGB);
//            drawCorresPoints(color_image, points_L1, points_R1, "test triangulation normalized points ", CV_RGB(255,0,255));

//            std::vector<cv::Vec3b> RGBValues;
//            for (unsigned int i = 0; i < points_L1.size(); ++i){
//                RGBValues.push_back(frame_L1.at<cv::Vec3b>(points_L1[i].x, points_L1[i].y));
//            }

//            std::vector<cv::Point3f> pCloudTest;
//            TriangulatePointsHZ(P0, P_LR, normP_L1, normP_R1, 0, pCloudTest);
//            int index = 0;
//            for (auto i : pCloudTest){
//                cout<< index << ":  " << i << endl;
//                ++index;
//            }

//            RunVisualization(pCloudTest, RGBValues);

//            cv::waitKey(0);
//            ++frame;
//            continue;
//        }

        // find inliers from median value
//        std::vector<cv::Point2f> inliersMedian_L1a, inliersMedian_R1a;
//        getInliersFromMedianValue(make_pair(corresPointsL1toR1.first, corresPointsL1toR1.second), &inliersMedian_L1a, &inliersMedian_R1a);

//        std::vector<cv::Point2f> inliersMedian_L1b, inliersMedian_L2;
//        getInliersFromMedianValue(make_pair(corresPointsL1toL2.first, corresPointsL1toL2.second), &inliersMedian_L1b, &inliersMedian_L2);

//        std::vector<cv::Point2f> inliersMedian_R1b, inliersMedian_R2;
//        getInliersFromMedianValue(make_pair(corresPointsR1toR2.first, corresPointsR1toR2.second), &inliersMedian_R1b, &inliersMedian_R2);

//        deleteZeroLines(inliersMedian_L1a, inliersMedian_R1a, inliersMedian_L1b, inliersMedian_L2, inliersMedian_R1b, inliersMedian_R2);

        if (8 > points_L1.size()) {
            cout << "to less points found" << endl;
            ++frame;
            continue;
        }

        // compute fundemental matrix FL1L2
        cv::Mat F_L;
        bool foundF_L;
        std::vector<cv::Point2f> inliersF_L1, inliersF_L2;
        foundF_L = getFundamentalMatrix(points_L1, points_L2, &inliersF_L1, &inliersF_L2, F_L);

        // compute fundemental matrix FL1L2
        cv::Mat F_R;
        bool foundF_R;
        std::vector<cv::Point2f> inliersF_R1, inliersF_R2;
        foundF_R = getFundamentalMatrix(points_R1, points_R2, &inliersF_R1, &inliersF_R2, F_R);

        // can't find fundamental Mat
        if (!foundF_L || !foundF_R){
            cout << "can't find F" << endl;
            ++frame;
            continue;
        }

        // make sure that there are this inlier in all frames. If not delete this inlier in all frames
        deleteZeroLines(inliersF_L1, inliersF_L2, inliersF_R1, inliersF_R2);

        //visualisize
        // convert grayscale to color image
        cv::Mat color_image;
        cv::cvtColor(frame_L1, color_image, CV_GRAY2RGB);
        drawCorresPoints(color_image, inliersF_L1, inliersF_L2, "inliers F L ", CV_RGB(0,255,0));
//        drawCorresPoints(color_image, inliersF_R1, inliersF_R2, "inliers F R ", CV_RGB(0,255,0));

//        drawCorresPoints(color_image, inliersMedianL1a, inliersMedianL2, "Found CorresPoints L1 To L2", CV_RGB(255,0,0));
//        drawCorresPoints(color_image, inliersMedianR1b, inliersMedianR2, "Found CorresPoints R1 To R2", CV_RGB(0,0,255));
//        drawCorresPoints(color_image, corresPoints1to2.first, corresPoints1to2.second, "Found CorresPoints", CV_RGB(0,255,0));
//        drawCorresPoints(color_image, corresPointsL1toR1.first, corresPointsL1toR1.second, "Found CorresPoints", CV_RGB(255,0,0));
//        drawCorresPoints(color_image, corresPointsL1toR2.first, corresPointsL1toR2.second, "Found CorresPoints", CV_RGB(0,0,255));

//        drawCorresPoints(color_image, inliersMedianL1, inliersMedianR1, "Inliers Median", CV_RGB(255,255,0));
//        drawCorresPoints(color_image, inliersFL1, inliersFR1, "inliers after ransac. for F computation", CV_RGB(0,255,255));
//        drawEpipolarLines(frame1L, frame1R, inliersFL1, inliersFR1, F);

        // normalisize all Points
        std::vector<cv::Point2f> normPoints_L1, normPoints_R1, normPoints_L2, normPoints_R2;
        normalizePoints(KInv_L, KInv_R, inliersF_L1, inliersF_R1, normPoints_L1, normPoints_R1);
        normalizePoints(KInv_L, KInv_R, inliersF_L2, inliersF_R2, normPoints_L2, normPoints_R2);



        // calculate essential mat
        cv::Mat E_L = K_R.t() * F_L * K_L; //according to HZ (9.12)
        cv::Mat E_R = K_R.t() * F_R * K_L; //according to HZ (9.12)

//        std::cout << "\n\n FundamentalMat \n" << F << std::endl;
//        std::cout << "\n\n FundamentalMat Test\n" << FTest << std::endl;
//        std::cout << "EssentialMat \n" << E << std::endl;
//        std::cout << "\n\n EssentialMat TEST \n" << ETest << std::endl;
//        cvWaitKey(0);

        // decompose right solution for R and T values and saved it to P1. get point cloud of triangulated points
        cv::Mat P_L, P_R;
        std::vector<cv::Point3f> pointCloud_L, pointCloud_R;
        bool goodPFound_L = getRightProjectionMat(E_L, P_L, normPoints_L1, normPoints_L2, pointCloud_L);
        bool goodPFound_R = getRightProjectionMat(E_R, P_R, normPoints_R1, normPoints_R2, pointCloud_R);

        if (goodPFound_L && goodPFound_R) {
            // find right scale factors u und v (according to rodehorst paper)
            // 1. method:
            double u_L1, u_R1;
            getScaleFactor(P0, P_LR, P_L, P_R, normPoints_L1, normPoints_R1, normPoints_L2, normPoints_R2, u_L1, u_R1);

            // 2. method:
            cv::Mat R_L, T_L, R_R, T_R;
            decomposeProjectionMat(P_L, R_L, T_L);
            decomposeProjectionMat(P_R, R_R, T_R);

            double u_L2, u_R2;
            getScaleFactor2(T_L, R_L, T_R, T_LR, R_LR, u_L2, u_R2);

            //compare both methods
//            cout << "u links  1: " << u_L1 << endl;
//            cout << "u rechts 1: " << u_R1 << endl << endl;
//            cout << "u links  2: " << u_L2 << endl;
//            cout << "u rechts 2: " << u_R2 << endl;

            // LEFT
            cv::Mat deltaPosL = (cv::Mat_<double>(4,4) <<
                                     R_L.at<double>(0,0),	R_L.at<double>(0,1),	R_L.at<double>(0,2),	T_L.at<double>(0),
                                     R_L.at<double>(1,0),	R_L.at<double>(1,1),	R_L.at<double>(1,2),	T_L.at<double>(1),
                                     R_L.at<double>(2,0),	R_L.at<double>(2,1),	R_L.at<double>(2,2),	T_L.at<double>(2),
                                     0                  , 0                    ,	0                  ,	1                   );

            positionL = positionL * deltaPosL;

            cv::Mat rotationL, translationL;
            decomposeProjectionMat(positionL, rotationL, translationL);

            std::stringstream left;
            left << "camera_left" << frame;
            addCameraToVisualizer(cv::Matx33f(rotationL),cv::Vec3f(translationL),0,125,0,0.2,left.str());


            // RIGHT
            cv::Mat deltaPosR = (cv::Mat_<double>(4,4) <<
                                     R_R.at<double>(0,0),	R_R.at<double>(0,1),	R_R.at<double>(0,2),	T_R.at<double>(0),
                                     R_R.at<double>(1,0),	R_R.at<double>(1,1),	R_R.at<double>(1,2),	T_R.at<double>(1),
                                     R_R.at<double>(2,0),	R_R.at<double>(2,1),	R_R.at<double>(2,2),	T_R.at<double>(2),
                                     0                  , 0                    ,	0                  ,	1                   );


            positionR = positionR * deltaPosR;

            cv::Mat rotationR, translationR;
            decomposeProjectionMat(positionR, rotationR, translationR);

            std::stringstream right;
            right << "camera_right" << frame;
            addCameraToVisualizer(cv::Matx33f(rotationR),cv::Vec3f(translationR),0,255,0,0.2,right.str());

            {   //second method (solvePnPRansac)
                // Triangulate stereo system:
                std::vector<cv::Point3f> worldcoordinates_LR;
                TriangulatePointsHZ(P0, P_LR, normPoints_L1, normPoints_R1, 0, worldcoordinates_LR);

                // SOLVE PNP RANSAC
                cv::Mat T2_L, T2_R, R2_L, R2_R;
                findPoseEstimation(P_L, K_L, worldcoordinates_LR, inliersF_L2, T2_L, R2_L);
                findPoseEstimation(P_R, K_R, worldcoordinates_LR, inliersF_R2, T2_R, R2_R);

                // LEFT 2
                cv::Mat deltaPos2L = (cv::Mat_<double>(4,4) <<
                                         R2_L.at<double>(0,0),	R2_L.at<double>(0,1),	R2_L.at<double>(0,2),	T2_L.at<double>(0),
                                         R2_L.at<double>(1,0),	R2_L.at<double>(1,1),	R2_L.at<double>(1,2),	T2_L.at<double>(1),
                                         R2_L.at<double>(2,0),	R2_L.at<double>(2,1),	R2_L.at<double>(2,2),	T2_L.at<double>(2),
                                         0                  , 0                    ,	0                  ,	1                   );

                position2L = position2L * deltaPos2L;

                cv::Mat rotation2L, translation2L;
                decomposeProjectionMat(position2L, rotation2L, translation2L);

                std::stringstream left2;
                left << "camera_left2" << frame;
                addCameraToVisualizer(cv::Matx33f(rotation2L),cv::Vec3f(translation2L),125,0,0,2.0,left2.str());


                // RIGHT 2
                cv::Mat deltaPos2R = (cv::Mat_<double>(4,4) <<
                                         R2_R.at<double>(0,0),	R2_R.at<double>(0,1),	R2_R.at<double>(0,2),	T2_R.at<double>(0),
                                         R2_R.at<double>(1,0),	R2_R.at<double>(1,1),	R2_R.at<double>(1,2),	T2_R.at<double>(1),
                                         R2_R.at<double>(2,0),	R2_R.at<double>(2,1),	R2_R.at<double>(2,2),	T2_R.at<double>(2),
                                         0                  , 0                    ,	0                  ,	1                   );


                position2R = position2R * deltaPos2R;

                cv::Mat rotation2R, translation2R;
                decomposeProjectionMat(position2R, rotation2R, translation2R);

                std::stringstream right2;
                right << "camera_right2" << frame;
                addCameraToVisualizer(cv::Matx33f(rotation2R),cv::Vec3f(translation2R),255,0,0,2.0,right2.str());
            }






            // get RGB values for pointcloud representation
            std::vector<cv::Vec3b> RGBValues;
            for (unsigned int i = 0; i < inliersF_L1.size(); ++i){
                RGBValues.push_back(frame_L1.at<cv::Vec3b>(inliersF_L1[i].x, inliersF_L1[i].y));
            }
            RunVisualization(pointCloud_L, frame, RGBValues);

            //visualisize
//            currentPos_L1 = drawCameraPath(path1, currentPos_L1, T_L * (u_L1/100.0), "motionPath 1", cv::Scalar(255,0,0));
//            currentPos_R1 = drawCameraPath(path1, currentPos_R1, T_R * (u_R1/100.0), "motionPath 1", cv::Scalar(0,255,0));
//            currentPos_L2 = drawCameraPath(path2, currentPos_L2, T_L * u_L2, "motionPath 2", cv::Scalar(255,0,0));
//            currentPos_R2 = drawCameraPath(path2, currentPos_R2, T_R * u_R2, "motionPath 2", cv::Scalar(0,255,0));


//            cv::Mat KNew, RNew, TNew, RotX, RotY, RotZ, EulerRot;
//            cv::decomposeProjectionMatrix(P_L, KNew, RNew, TNew, RotX, RotY, RotZ, EulerRot);
//            cout << P_L << endl << RNew << endl << TNew << endl;

//            double n = TNew.at<double>(3,0);
//            double x = TNew.at<double>(0,0)/n;
//            double y = TNew.at<double>(1,0)/n;
//            double z = TNew.at<double>(2,0)/n;

//            cv::Vec3f TVec(x, y, z);
//            cv::Vec3f TVecTest(T_LR);

//            double length1 = sqrt(TVec[0] * TVec[0] + TVec[1] * TVec[1] + TVec[2] *TVec[2] );
//            double length2 = sqrt(TVecTest[0] * TVecTest[0] + TVecTest[1] * TVecTest[1] + TVecTest[2] *TVecTest[2] );

//            cout << "cameraRot: owndata " << endl << EulerRot << endl;
//            cout << "cameraPos: owndata [" << TVec[0]/length1 << ", " << TVec[1]/length1 << ", " << TVec[2]/length1 << "]"  << endl;
//            cout << "cameraPos: hagen   [" << TVecTest[0]/length2 << ", " << TVecTest[1]/length2 << ", " << TVecTest[2]/length2 << "]"   << endl;
//            cout << "cameraRot: hagen " << endl << RTest << endl;

        } else {
            cout << "can't estimate motion no perspective Mat Found" << endl;
        }

//        {   //second method (solvePnPRansac)
//            cv::Mat T2_L, T2_R;
//            method2(normPoints_L1, normPoints_R1, normPoints_L2, normPoints_R2, P_LR, K_L, K_R, T2_L, T2_R);
//            currentPos_L3 = drawCameraPath(path3, currentPos_L3, cv::Mat(T2_L*0.000001), "motionPath 3", cv::Scalar(255,0,0));
//            currentPos_R3 = drawCameraPath(path3, currentPos_R3, cv::Mat(T2_R*0.000001), "motionPath 3", cv::Scalar(0,255,0));
//        }

//        {   //third method (haagen)
//            cv::Mat T3;
//            method3(normPoints_L1, normPoints_R1, normPoints_L2, normPoints_R2, P_LR, T3);
//            currentPos_4 = drawCameraPath(path4, currentPos_4, cv::Mat(T3), "motionPath 4", cv::Scalar(255,0,0));
//        }

        ++frame;
    }
    cv::waitKey(0);
    return 0;
}

void method3 (const std::vector<cv::Point2f> &point_L1, const std::vector<cv::Point2f>& point_R1, const std::vector<cv::Point2f>& point_L2, const std::vector<cv::Point2f>& point_R2, const cv::Mat& P_LR, cv::Mat& T){
    cv::Mat P0 = (cv::Mat_<double>(3,4) <<
                   1.0, 0.0, 0.0, 0.0,
                   0.0, 1.0, 0.0, 0.0,
                   0.0, 0.0, 1.0, 0.0 );

    std::vector<cv::Point3f> worldcoordinates_1, worldcoordinates_2;
    TriangulatePointsHZ(P0, P_LR, point_L1, point_R1, 0, worldcoordinates_1);
    TriangulatePointsHZ(P0, P_LR, point_L2, point_R2, 0, worldcoordinates_2);

    std::vector<cv::Point3f> translationVectors;
    for (unsigned int i = 0; i < worldcoordinates_1.size(); ++i){
        translationVectors.push_back(worldcoordinates_1[i] - worldcoordinates_2[i]);
    }

    cv::Point3f transVec = accumulate(translationVectors.begin(), translationVectors.end(), cv::Point3f(0,0,0), [](const cv::Point3f& sum, const cv::Point3f& p) {
        return sum+p;
    });

    cout << transVec<< endl;
    transVec = (1.0/translationVectors.size()) * transVec ;
    cout << transVec<< endl;

    T = cv::Mat(transVec);
}

















