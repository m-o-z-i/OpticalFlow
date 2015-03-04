#include <opencv2/opencv.hpp>

#include "line/MyLine.h"

#include <cmath>
#include <math.h> 
#include <vector>
#include <utility>

#include <sstream>
#include <string.h>

#include <opencv2/core/core.hpp>
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
//#include "opencv2/nonfree/nonfree.hpp"

using namespace std;

static const double pi = 3.14159265358979323846;

inline static double square(int a)
{
	return a * a;
}

char key;

void drawLine(IplImage* ref, cv::Point2f p, cv::Point2f q, float angle, cv::Scalar const& color = CV_RGB(0,0,0), int line_thickness = 1);
void drawLine(cv::Mat ref, cv::Point2f p, cv::Point2f q, float angle, cv::Scalar const& color = CV_RGB(0,0,0), int line_thickness = 1);
void drawPoints (cv::Mat const& image, vector<cv::Point2f> points, string windowName, cv::Scalar const& color = CV_RGB(0,0,0));


std::vector<cv::Point2f> getStrongFeaturePoints (cv::Mat const& image, int number = 50, float minQualityLevel = .03, float minDistance = 0.1);
pair<vector<cv::Point2f>, vector<cv::Point2f> > refindFeaturePoints(cv::Mat const& prev_image, cv::Mat const& next_image, vector<cv::Point2f> frame1_features);
void drawEpipolarLines(cv::Mat frame1, cv::Mat frame2, vector<cv::Point2f> const& points1, vector<cv::Point2f> const& points2);
void drawCorresPoints(cv::Mat const& image, vector<cv::Point2f> inliers1, vector<cv::Point2f> inliers2, cv::Scalar const& color);
void drawOptFlowMap (const cv::Mat& flow, cv::Mat& cflowmap, int step, const cv::Scalar& color);


void getInliersFromMeanValue (pair<vector<cv::Point2f>, vector<cv::Point2f>> const& features, vector<cv::Point2f> *inliers2, vector<cv::Point2f> *inliers1);
void getInliersFromFundamentalMatrix(pair<vector<cv::Point2f>, vector<cv::Point2f>> const& points, vector<cv::Point2f> *inliers1, vector<cv::Point2f> *inliers2);



//TODO: calcOpticalFlowFarneback
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
 *
 *
 * surf detection need nonfree lib... can't use it in the vr-lab
 */

int main() {
    int frame=1;

	while(true)
    {
        //stereo1
        cv::Mat mat_image11 = cv::imread("data/stereoImages/left/"+(std::to_string(frame))+"_l.jpg",0);
        cv::Mat mat_image12 = cv::imread("data/stereoImages/right/"+(std::to_string(frame))+"_r.jpg",0);

        //stereo2
        cv::Mat mat_image21 = cv::imread("data/stereoImages/left/"+(std::to_string(frame+1))+"_l.jpg",0);
        cv::Mat mat_image22 = cv::imread("data/stereoImages/right/"+(std::to_string(frame+1))+"_r.jpg",0);

        //cv::namedWindow("OpticalFlow", cv::WINDOW_AUTOSIZE);

        // Check for invalid input
        if(! mat_image11.data || !mat_image12.data || !mat_image22.data || !mat_image21.data)
        {
            cout <<  "Could not open or find the image: "  << std::endl ;
            break;
        }

        vector<cv::Point2f> features1 = getStrongFeaturePoints(mat_image11, 150,0.01,5);
        drawPoints(mat_image11, features1, "1_left_features", cv::Scalar(0,0,0));

        pair<vector<cv::Point2f>, vector<cv::Point2f>> corresPoints1 = refindFeaturePoints(mat_image11, mat_image12, features1);
        drawPoints(mat_image12, corresPoints1.second, "corres points in Frame12", cv::Scalar(0,0,0));
        std::cout << "Frame: "<< frame << " found " << features1.size() << " features and " << corresPoints1.first.size() << "  corres Points " << std::endl;

        //pair<vector<cv::Point2f>, vector<cv::Point2f>> corresPoints2 = refindFeaturePoints(mat_image11, mat_image21, features1);
        //drawPoints(mat_image12, corresPoints2.second, "corresPoints in Frame21", cv::Scalar(0,255,255));

        //pair<vector<cv::Point2f>, vector<cv::Point2f>> corresPoints3 = refindFeaturePoints(mat_image11, mat_image22, features1);
        //drawPoints(mat_image12, corresPoints3.second, "corresPoints in Frame22", cv::Scalar(0,255,255));

        vector<cv::Point2f> inliersM1, inliersM2;
        getInliersFromMeanValue(corresPoints1, &inliersM1, &inliersM2);
        std::cout << "deltete  " << corresPoints1.first.size() - inliersM1.size() << " outliers Points from mean value " << std::endl;
        drawPoints(mat_image12, inliersM2, "inliers by mean in Frame12", cv::Scalar(0,255,0));

        drawEpipolarLines(mat_image11, mat_image12, inliersM1, inliersM2);

        vector<cv::Point2f> inliersF1, inliersF2;
        getInliersFromFundamentalMatrix(corresPoints1, &inliersF1, &inliersF2);
        std::cout << "deltete  " << corresPoints1.first.size() - inliersF1.size() << " outliers Points from fumdamentalmatrix " << std::endl;
        drawPoints(mat_image12, inliersF2, "inliers by fundamental in Frame12", cv::Scalar(255,255,0));


        drawCorresPoints(mat_image11, inliersM1, inliersM1, cv::Scalar(255,0,0) );

                // Motion Estimation
        // Get Matrix K
        // calculate EssentialMatrix
        // for bundle adjustment use SSBA
        // or http://stackoverflow.com/questions/13921720/bundle-adjustment-functions
        // recover Pose (need newer version of calib3d)


        cv::Mat flow, cflow;
        cv::calcOpticalFlowFarneback(mat_image11, mat_image21, flow, 0.5, 3, 15, 3, 5, 1.2, 0);
        cv::cvtColor(mat_image11, cflow, CV_GRAY2BGR);
        drawOptFlowMap(flow, cflow, 50, CV_RGB(0, 255, 0));
        cv::imshow("optical Flow", cflow);


        ++frame;
        cvWaitKey();
	}
	return 0;
}

void drawOptFlowMap (const cv::Mat& flow, cv::Mat& cflowmap, int step, const cv::Scalar& color) {
    for(int y = 0; y < cflowmap.rows; y += step) {
        for(int x = 0; x < cflowmap.cols; x += step)
        {
            const cv::Point2f& fxy = flow.at<cv::Point2f>(y, x);
            cv::line(cflowmap, cv::Point(x,y), cv::Point(cvRound(x+fxy.x), cvRound(y+fxy.y)),
                 color);
            cv::circle(cflowmap, cv::Point(cvRound(x+fxy.x), cvRound(y+fxy.y)), 1, color, -1);
        }
    }
}

vector<cv::Point2f> getStrongFeaturePoints(const cv::Mat& image, int number, float minQualityLevel, float minDistance) {
    /* Shi and Tomasi Feature Tracking! */

    /* Preparation: This array will contain the features found in image1. */
    vector<cv::Point2f> image_features;

    /* Preparation: BEFORE the function call this variable is the array size
     * (or the maximum number of features to find).  AFTER the function call
     * this variable is the number of features actually found.
     */
    int number_of_features = number;

    /* Actually run the Shi and Tomasi algorithm!!
     * "Image" is the input image.
     * qualityLevel specifies the minimum quality of the features (based on the eigenvalues)
     * minDistance specifies the minimum Euclidean distance between features
     * RETURNS:
     * "image_features" will contain the feature points.
     */
    cv::goodFeaturesToTrack(image, image_features, number_of_features, minQualityLevel, minDistance);
    return image_features;
}

pair<vector<cv::Point2f>, vector<cv::Point2f>> refindFeaturePoints(const cv::Mat& prev_image, const cv::Mat& next_image, vector<cv::Point2f> frame1_features){
    /* Pyramidal Lucas Kanade Optical Flow! */

    /* This array will contain the locations of the points from frame 1 in frame 2. */
    vector<cv::Point2f>  frame2_features;

    /* The i-th element of this array will be non-zero if and only if the i-th feature of
     * frame 1 was found in frame 2.
     */
    vector<unsigned char> optical_flow_found_feature;

    /* The i-th element of this array is the error in the optical flow for the i-th feature
     * of frame1 as found in frame 2.  If the i-th feature was not found (see the array above)
     * I think the i-th entry in this array is undefined.
     */
    vector<float> optical_flow_feature_error;

    /* This is the window size to use to avoid the aperture problem (see slide "Optical Flow: Overview"). */
    CvSize optical_flow_window = cvSize(15,15);

    /* 0-based maximal pyramid level number; if set to 0, pyramids are not used (single level),
     * if set to 1, two levels are used, and so on; if pyramids are passed to input then algorithm
     * will use as many levels as pyramids have but no more than maxLevel.
     * */
    int maxLevel = 10;

    /* This termination criteria tells the algorithm to stop when it has either done 20 iterations or when
     * epsilon is better than .3.  You can play with these parameters for speed vs. accuracy but these values
     * work pretty well in many situations.
     */
    cv::TermCriteria optical_flow_termination_criteria
        = cv::TermCriteria( cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 20, .3 );

    /* Actually run Pyramidal Lucas Kanade Optical Flow!!
     * "prev_image" is the first frame with the known features. pyramid constructed by buildOpticalFlowPyramid()
     * "next_image" is the second frame where we want to find the first frame's features.
     * "frame1_features" are the features from the first frame.
     * "frame2_features" is the (outputted) locations of those features in the second frame.
     * "number_of_features" is the number of features in the frame1_features array.
     * "optical_flow_window" is the size of the window to use to avoid the aperture problem.
     * "maxLevel" is the maximum number of pyramids to use.  0 would be just one level.
     * "optical_flow_found_feature" is as described above (non-zero iff feature found by the flow).
     * "optical_flow_feature_error" is as described above (error in the flow for this feature).
     * "optical_flow_termination_criteria" is as described above (how long the algorithm should look).
     * "0" means disable enhancements.  (For example, the second array isn't pre-initialized with guesses.)
     */
    //TODO: improve TermCriteria. do not quit program when it is reached
    cv::calcOpticalFlowPyrLK(prev_image, next_image, frame1_features, frame2_features, optical_flow_found_feature,
                             optical_flow_feature_error, optical_flow_window, maxLevel,
                             optical_flow_termination_criteria, cv::OPTFLOW_LK_GET_MIN_EIGENVALS);


    vector<cv::Point2f>::iterator iter_f1 = frame1_features.begin();
    vector<cv::Point2f>::iterator iter_f2 = frame2_features.begin();
    for (unsigned i = 0; i < frame1_features.size(); ++i){
        if ( optical_flow_found_feature[i] == 0 ){
            frame1_features.erase(iter_f1);
            frame2_features.erase(iter_f2);
        }
        ++iter_f1;
        ++iter_f2;
    }


    return make_pair(frame1_features, frame2_features);
}

void getInliersFromMeanValue (const pair<vector<cv::Point2f>, vector<cv::Point2f> >& features, vector<cv::Point2f>* inliers1, vector<cv::Point2f>* inliers2){
    vector<double> directions;
    vector<double> lengths;

    for (unsigned i = 0; i < features.first.size(); ++i){
        double direction = atan2( (double) features.first[i].y - features.second[i].y, (double) features.second[i].x - features.second[i].x );
        directions.push_back(direction);

        double length = sqrt( square(features.first[i].y - features.second[i].y) + square(features.first[i].x - features.second[i].x) );
        lengths.push_back(length);
    }

    sort(directions.begin(), directions.end());
    double median_angle = directions[(int)(directions.size()/2)];

    sort(lengths.begin(),lengths.end());
    double median_lenght = lengths[(int)(lengths.size()/2)];

    for(unsigned i = 0; i < features.first.size(); ++i)
    {
        double direction = atan2( (double) features.first[i].y - features.second[i].y, (double) features.second[i].x - features.second[i].x );
        double length = sqrt( square(features.first[i].y - features.second[i].y) + square(features.first[i].x - features.second[i].x) );

        if (direction < median_angle + 2 && direction > median_angle - 2 ) {
            if (length < (median_lenght*3) && length > 1.5 && length > median_lenght*0.1) {
                inliers1->push_back(features.first[i]);
                inliers2->push_back(features.second[i]);
            }
        }
    }
}

void getInliersFromFundamentalMatrix(pair<vector<cv::Point2f>, vector<cv::Point2f>> const& points, vector<cv::Point2f> *inliers1, vector<cv::Point2f> *inliers2) {
    // Compute F matrix using RANSAC
    if(points.first.size() != points.second.size()){
        return;
    }
    std::vector<uchar> inliers_fundamental(points.first.size(),0);
    cv::Mat fundemental = cv::findFundamentalMat(
                          cv::Mat(points.first), cv::Mat(points.second),   // matching points
                          inliers_fundamental,                             // match status (inlier ou outlier)
                          cv::FM_RANSAC,                                   // RANSAC method
                          1,                                               // distance to epipolar line
                          0.98);                                           // confidence probability

    //get Inlier
    std::vector<uchar>::const_iterator itIn = inliers_fundamental.begin();
    for(unsigned i = 0; i<points.first.size(); ++i){
        // draw a circle at each inlier location
        if (inliers_fundamental[i] == 1) {
            inliers1->push_back(points.first[i]);
            inliers2->push_back(points.second[i]);
        }
    }
}

void drawEpipolarLines(cv::Mat frame1, cv::Mat frame2, const vector<cv::Point2f>& points1, const vector<cv::Point2f>& points2) {
    cv::Mat mat_color1;
    cv::Mat mat_color2;

    cv::cvtColor(frame1, mat_color1, CV_GRAY2RGB);
    cv::cvtColor(frame2, mat_color2, CV_GRAY2RGB);

    // Compute F matrix using RANSAC
    if (points1.size()>10 && points2.size()>10){
        std::cout << "fundamental: " << points1.size() << " " << points2.size() << " " << cv::Mat(points1).rows << std::endl;
        std::vector<uchar> inliers_fundamental(points1.size(),0);
        cv::Mat fundemental = cv::findFundamentalMat(
                              cv::Mat(points1), cv::Mat(points2),   // matching points
                              inliers_fundamental,                  // match status (inlier ou outlier)
                              cv::FM_RANSAC,                          // RANSAC method
                              1,                                    // distance to epipolar line
                              0.98);                                // confidence probability

        std::vector<cv::Vec3f> lines1;
        cv::computeCorrespondEpilines(cv::Mat(points1),1,fundemental,lines1);
        for (vector<cv::Vec3f>::const_iterator it= lines1.begin();
             it!=lines1.end(); ++it) {

                 cv::line(frame2,cv::Point(0,-(*it)[2]/(*it)[1]),
                                 cv::Point(frame2.cols,-((*it)[2]+(*it)[0]*frame2.cols)/(*it)[1]),
                                 cv::Scalar(255,255,255));
        }

        std::vector<cv::Vec3f> lines2;
        cv::computeCorrespondEpilines(cv::Mat(points2),2,fundemental,lines2);
        for (vector<cv::Vec3f>::const_iterator it= lines2.begin();
             it!=lines2.end(); ++it) {

                 cv::line(frame1,cv::Point(0,-(*it)[2]/(*it)[1]),
                                 cv::Point(frame1.cols,-((*it)[2]+(*it)[0]*frame1.cols)/(*it)[1]),
                                 cv::Scalar(255,255,255));
        }

        // Draw the inlier points
        std::vector<cv::Point2f> points1In, points2In;
        std::vector<cv::Point2f>::const_iterator itPts= points1.begin();
        std::vector<uchar>::const_iterator itIn= inliers_fundamental.begin();
        while (itPts!=points1.end()) {

            // draw a circle at each inlier location
            if (*itIn ) {
                cv::circle(frame1,*itPts,3,cv::Scalar(0,255,0),2);
                points1In.push_back(*itPts);
            }
            ++itPts;
            ++itIn;
        }

        itPts= points2.begin();
        itIn= inliers_fundamental.begin();
        while (itPts!=points2.end()) {

            // draw a circle at each inlier location
            if (*itIn) {
                cv::circle(frame2,*itPts,3,cv::Scalar(0,255,0),2);
                points2In.push_back(*itPts);
            }
            ++itPts;
            ++itIn;
        }

        // Display the images with points
        cv::namedWindow("Right Image Epilines (RANSAC)", cv::WINDOW_NORMAL);
        cv::imshow("Right Image Epilines (RANSAC)",frame1);
        cv::namedWindow("Left Image Epilines (RANSAC)", cv::WINDOW_NORMAL);
        cv::imshow("Left Image Epilines (RANSAC)",frame2);


        std::vector<uchar> inliers_homographie(points1.size(),0);
        cv::findHomography(cv::Mat(points1In),cv::Mat(points2In),inliers_homographie,CV_RANSAC,1.);
        // Draw the homography inlier points
        itPts= points1In.begin();
        itIn= inliers_homographie.begin();
        cout << "Homography:  " << points1In.size() << " " << points2In.size() << endl;
        while (itPts!=points1In.end()) {

            // draw a circle at each inlier location
            if (*itIn)
                cv::circle(mat_color1,*itPts,3,cv::Scalar(0,255,0),2);
            else {
                cv::circle(mat_color1,*itPts,3,cv::Scalar(0,0,255),2);
            }

            ++itPts;
            ++itIn;
        }

        itPts= points2In.begin();
        itIn= inliers_homographie.begin();
        while (itPts!=points2In.end()) {

            // draw a circle at each inlier location
            if (*itIn)
                cv::circle(mat_color2,*itPts,3,cv::Scalar(0,255,0),2);
            else {
                cv::circle(mat_color2,*itPts,3,cv::Scalar(0,0,255),2);
            }

            ++itPts;
            ++itIn;
        }

        if (inliers_homographie.size() > inliers_fundamental.size()){
            cout << "skip frame because points are on one plane" << endl;
        }

        // Display the images with points
        cv::namedWindow("Right Image Homography (RANSAC)", cv::WINDOW_NORMAL);
        cv::imshow("Right Image Homography (RANSAC)",mat_color1);
        cv::namedWindow("Left Image Homography (RANSAC)", cv::WINDOW_NORMAL);
        cv::imshow("Left Image Homography (RANSAC)",mat_color2);

        // SAVE IMAGEs
        //string path = "data/image/epipoles/current"+(to_string(frame))+".png";
        //imwrite(path.c_str(), mat_image1);
        //cv::waitKey();
    }
}

void drawCorresPoints(cv::Mat const& image, vector<cv::Point2f> inliers1, vector<cv::Point2f> inliers2, cv::Scalar const& color) {
    // convert grayscale to color image
    cv::Mat color_image;
    cv::cvtColor(image, color_image, CV_GRAY2RGB);

    for(unsigned int i = 0; i < inliers1.size(); i++)
    {
        double angle;		angle = atan2( (double) inliers1[i].y - inliers2[i].y, (double) inliers1[i].x - inliers2[i].x );
        double hypotenuse;	hypotenuse = sqrt( square(inliers1[i].y - inliers2[i].y) + square(inliers1[i].x - inliers2[i].x) );

        /* Here we lengthen the arrow by a factor of three. */
        inliers2[i].x = (int) (inliers1[i].x - hypotenuse * cos(angle));
        inliers2[i].y = (int) (inliers1[i].y - hypotenuse * sin(angle));

        drawLine(color_image, inliers1[i], inliers2[i], angle, CV_RGB(color[0], color[1], color[2]));
    }


    /* Now display the image we drew on.  Recall that "Optical Flow" is the name of
     * the window we created above.
     */
    cv::imshow("OpticalFlow", color_image);
}

void drawLine (cv::Mat ref, cv::Point2f p, cv::Point2f q, float angle, const cv::Scalar& color, int line_thickness ) {
    /* Now we draw the main line of the arrow. */
    /* "frame1" is the frame to draw on.
     * "p" is the point where the line begins.
     * "q" is the point where the line stops.
     * "CV_AA" means antialiased drawing.
     * "0" means no fractional bits in the center cooridinate or radius.
     */
    cv::line( ref, p, q, color, line_thickness, CV_AA, 0 );
    /* Now draw the tips of the arrow.  I do some scaling so that the
     * tips look proportional to the main line of the arrow.
     */
    p.x = (int) (q.x + 9 * cos(angle + pi / 4));
    p.y = (int) (q.y + 9 * sin(angle + pi / 4));
    cv::line( ref, p, q, color, line_thickness, CV_AA, 0 );
    p.x = (int) (q.x + 9 * cos(angle - pi / 4));
    p.y = (int) (q.y + 9 * sin(angle - pi / 4));
    cv::line( ref, p, q, color, line_thickness, CV_AA, 0 );
}

void drawLine (IplImage* ref, cv::Point2f p, cv::Point2f q, float angle, const cv::Scalar& color, int line_thickness ) {
    /* Now we draw the main line of the arrow. */
    /* "frame1" is the frame to draw on.
     * "p" is the point where the line begins.
     * "q" is the point where the line stops.
     * "CV_AA" means antialiased drawing.
     * "0" means no fractional bits in the center cooridinate or radius.
     */
    cvLine( ref, p, q, color, line_thickness, CV_AA, 0 );
    /* Now draw the tips of the arrow.  I do some scaling so that the
     * tips look proportional to the main line of the arrow.
     */
    p.x = (int) (q.x + 9 * cos(angle + pi / 4));
    p.y = (int) (q.y + 9 * sin(angle + pi / 4));
    cvLine( ref, p, q, color, line_thickness, CV_AA, 0 );
    p.x = (int) (q.x + 9 * cos(angle - pi / 4));
    p.y = (int) (q.y + 9 * sin(angle - pi / 4));
    cvLine( ref, p, q, color, line_thickness, CV_AA, 0 );
}

void drawPoints (cv::Mat const& image, vector<cv::Point2f> points, string windowName, cv::Scalar const& color) {
    cv::Mat colorImg;
    cv::cvtColor(image, colorImg, CV_GRAY2RGB);
    for (auto i : points) {
        // draw a circle at each inlier location
        cv::circle(colorImg,i,3,color,1);
    }
    cv::imshow(windowName, colorImg);
}
